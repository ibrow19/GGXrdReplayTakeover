#include <replay-record.h>
#include <replay-mods.h>
#include <xrd-module.h>
#include <asw-engine.h>
#include <entity.h>
#include <input.h>
#include <common.h>
#include <cassert>

DEFINE_PROFILING_CATEGORY(ReplayRecord_SetFrame)

ReplayRecord::ReplayRecord()
: mRecordedFrames(0),
  mCurrentFrame(0)
{}

void ReplayRecord::Reset()
{
    mRecordedFrames = 0;
    mCurrentFrame = 0;
}

size_t ReplayRecord::RecordFrame()
{
    assert(mCurrentFrame <= mRecordedFrames);

    if (mRecordedFrames != 0 && mCurrentFrame < (MaxRoundFrames - 1))
    {
        ++mCurrentFrame;
    }

    if (mRecordedFrames == MaxRoundFrames)
    {
        return mCurrentFrame;
    }

    if (mRecordedFrames == mCurrentFrame)
    {
        ++mRecordedFrames;
        if (mCurrentFrame % SaveStateSpacing == 0)
        {
            ReplaySaveData& saveData = mSpacedBuffer[mCurrentFrame / SaveStateSpacing];
            SaveState(saveData);
            InputManager inputRecord = XrdModule::GetInputManager();
            saveData.p1ReplayPosition = inputRecord.GetP1ReplayPos();
            saveData.p2ReplayPosition = inputRecord.GetP2ReplayPos();
        }
    }

    return mCurrentFrame;
}

size_t ReplayRecord::SetFrame(size_t index, bool bForceLoad)
{
    SCOPE_COUNTER(ReplayRecord_SetFrame)

    if (index >= MaxRoundFrames)
    {
        index = MaxRoundFrames - 1;
    }

    if ((!bForceLoad  && index == mCurrentFrame) || (bForceLoad && mRecordedFrames == 0))
    {
        return mCurrentFrame;
    }

    // For rewinding find closest state before simulating forward to it.
    // If we are going to a further forward frame then only jump to the
    // spaced buffer first if we are force loading as we would prefer
    // to simulate forward where possible.
    if (mRecordedFrames > 0)
    {
        size_t availableSpacedIndex = index / SaveStateSpacing;
        if (index >= mRecordedFrames)
        {
            availableSpacedIndex = (mRecordedFrames - 1) / SaveStateSpacing;
        }

        size_t availableIndex = availableSpacedIndex * SaveStateSpacing;
        if (bForceLoad || index < mCurrentFrame)
        {
            ReplaySaveData& saveData = mSpacedBuffer[availableSpacedIndex];
            ReplayDetourSettings::bOverrideSimpleActorPause = true;
            LoadState(saveData);
            ReplayDetourSettings::bOverrideSimpleActorPause = false;
            InputManager inputRecord = XrdModule::GetInputManager();
            inputRecord.SetP1ReplayPos(saveData.p1ReplayPosition);
            inputRecord.SetP2ReplayPos(saveData.p2ReplayPosition);
            mCurrentFrame = availableIndex;
        }
    }

    // Simulate forward to desired frame.
    if (mCurrentFrame != index)
    {
        // Override pausing and disable sound effects/input display while simulating
        ReplayDetourSettings::bOverrideSimpleActorPause = true;
        DisableSoundEffects();
        DisableInputDisplay();

        // Clear input before simulating. Restore it after the simulation.
        GameInputCollection input = XrdModule::GetGameInput();
        DWORD& menuPressed = input.GetP1MenuInput().GetPressedMask();
        DWORD& menuHeld = input.GetP1MenuInput().GetHeldMask();
        DWORD& battlePressed = input.GetP1BattleInput().GetPressedMask();
        DWORD& battleHeld = input.GetP1BattleInput().GetHeldMask();
        DWORD oldMenuPressed = menuPressed;
        DWORD oldMenuHeld = menuHeld;
        DWORD oldBattlePressed = battlePressed;
        DWORD oldBattleHeld = battleHeld;
        menuPressed = 0;
        menuHeld = 0;
        battlePressed = 0;
        battleHeld = 0;

        // Relevant actors include entity's simple actors but also unrelated
        // actors like hitspark or RC vfx.
        TickRelevantActorsFunc tickRelevantActors = XrdModule::GetTickRelevantActors();
        MainGameLogicFunc tickGame = XrdModule::GetOfflineMainGameLogic();
        GameLogicManager manager = XrdModule::GetEngine().GetGameLogicManager();
        LPVOID managerPtr = (LPVOID)manager.GetPtr();
        DWORD battleCamera = manager.GetBattleCamera();

        // Camera update function is unreal script so we need to look up the
        // function and execute it with special functions.
        FindFunctionCheckedFunc findFunction = XrdModule::GetFindFunctionChecked();
        DWORD updateCameraUnrealScript = findFunction(battleCamera, XrdModule::GetTickFunctionFName(), XrdModule::GetTickFunctionGlobal(), 0);
        
        typedef void(__thiscall* UnrealScriptFunc)(DWORD thisArg, DWORD func, float* delta, DWORD boolParam);
        DWORD cameraVTable = *(DWORD*)battleCamera;
        UnrealScriptFunc executeUnrealScript = *(UnrealScriptFunc*)(cameraVTable + 0x108);

        // If this actor is not ticked then cinematic camera blends in
        // special camera update will accumulate incorrectly during resimulation.
        // (Not sure yet exactly what this actor is for)
        typedef void(__thiscall* TickAActorFunc)(DWORD thisArg, float delta, DWORD tickType);
        DWORD viewActor = manager.GetViewActor();
        DWORD viewActorVTable = *(DWORD*)(viewActor);
        TickAActorFunc tickViewActor = *(TickAActorFunc*)(viewActorVTable + 0x1a4);

        while (mCurrentFrame != index)
        {
            tickGame(managerPtr, 1);
            tickRelevantActors();

            float cameraDelta = 0.02f;
            tickViewActor(viewActor, cameraDelta, 2 /* TickType_All */);
            executeUnrealScript(battleCamera, updateCameraUnrealScript, &cameraDelta, 0);

            if (XrdModule::GetPreOrPostBattle())
            {
                break;
            }
            RecordFrame();
        }

        // Update players and other complex actors to the correct animation
        // state after we've finished advancing the game state.
        ForEachEntity([](DWORD entityPtr, DWORD index, void* extraData)
            {
                Entity entity(entityPtr);
                if (entity.GetComplexActor() && entity.GetIsNotInCutscene())
                {
                    UpdateAnimationFunc updateAnim = XrdModule::GetUpdateComplexActorAnimation();
                    updateAnim(entityPtr, entity.GetAnimationFrameName(), 1);
                }
            });

        // Restore original input.
        // If we don't restore the inputs then there are weird side effects 
        // like the game thinking you pressed certain buttons for the first time
        // next frame since we had a bunch of simulated frames where it wasn't pressd
        menuPressed = oldMenuPressed;
        menuHeld = oldMenuHeld;
        battlePressed = oldBattlePressed;
        battleHeld = oldBattleHeld;

        ReplayDetourSettings::bOverrideSimpleActorPause = false;
        EnableSoundEffects();
        EnableInputDisplay();
    }

    return mCurrentFrame;
}

size_t ReplayRecord::SetNextFrame()
{
    return SetFrame(mCurrentFrame + 1);
}

size_t ReplayRecord::SetPreviousFrame()
{
    if (mCurrentFrame == 0)
    {
        return mCurrentFrame;
    }
    return SetFrame(mCurrentFrame - 1);
}

size_t ReplayRecord::GetCurrentFrame() const
{
    return mCurrentFrame;
}
