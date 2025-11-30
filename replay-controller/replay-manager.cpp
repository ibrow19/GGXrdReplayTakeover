#include <replay-manager.h>
#include <xrd-module.h>
#include <asw-engine.h>
#include <entity.h>
#include <input.h>
#include <common.h>
#include <cassert>

ReplayManager::ReplayManager()
: mRecordedFrames(0),
  mCurrentFrame(0)
{}

void ReplayManager::Reset()
{
    mRecordedFrames = 0;
    mCurrentFrame = 0;
}

size_t ReplayManager::RecordFrame()
{
    assert(mCurrentFrame <= mRecordedFrames);
    if (mRecordedFrames != 0)
    {
        ++mCurrentFrame;
    }

    if (mRecordedFrames == mCurrentFrame)
    {
        ++mRecordedFrames;
        assert(mRecordedFrames < MaxRoundFrames);
        if (mCurrentFrame % SaveStateSpacing == 0)
        {
            ReplaySaveData& saveData = mSpacedBuffer[mCurrentFrame / SaveStateSpacing];
            SaveState(saveData);
            InputManager inputManager = XrdModule::GetInputManager();
            saveData.p1ReplayPosition = inputManager.GetP1ReplayPos();
            saveData.p2ReplayPosition = inputManager.GetP2ReplayPos();
        }
    }

    return mCurrentFrame;
}

// In the function responsible for playing sound effects: change a jump-if-equal
// near the start of the function to an unconditional jump to the end of the
// function. This will prevent all sound effects from playing. There are almost
// certainly better ways of doing this; especially since rollback resimulations
// presumably disable sound effects somehow. Regardless, the current solution is
// simple and works for what we currently need it to do.
static void DisableSoundEffects()
{
    BYTE* instruction = XrdModule::GetSoundEffectJumpInstruction();
    static bool bMemoryWritable = false;
    if (!bMemoryWritable)
    {
        MakeRegionWritable((DWORD)instruction, 6);
        bMemoryWritable = true;
    }

    // jmp + a nop for last byte
    instruction[0] = 0xe9;
    instruction[1] = 0x56;
    instruction[2] = 0x04;
    instruction[3] = 0x00;
    instruction[4] = 0x00;
    instruction[5] = 0x90;
}

static void ReEnableSoundEffects()
{
    // Restore je
    BYTE* instruction = XrdModule::GetSoundEffectJumpInstruction();
    instruction[0] = 0x0f;
    instruction[1] = 0x84;
    instruction[2] = 0x55;
    instruction[3] = 0x04;
    instruction[4] = 0x00;
    instruction[5] = 0x00;
}

// TODO: need to rename this since we don't always load now.
size_t ReplayManager::LoadFrame(size_t index, bool bForceLoad)
{
    if (index >= MaxRoundFrames)
    {
        index = MaxRoundFrames - 1;
    }

    if ((!bForceLoad  && index == mCurrentFrame) || (bForceLoad && mRecordedFrames == 0))
    {
        return mCurrentFrame;
    }

    // Find closest state before simulating forward to it.
    if (mRecordedFrames > 0)
    {
        size_t availableSpacedIndex = index / SaveStateSpacing;
        if (index >= mRecordedFrames)
        {
            availableSpacedIndex = (mRecordedFrames - 1) / SaveStateSpacing;
        }

        size_t availableIndex = availableSpacedIndex * SaveStateSpacing;
        if (bForceLoad || index < mCurrentFrame || availableIndex > mCurrentFrame)
        {
            ReplaySaveData& saveData = mSpacedBuffer[availableSpacedIndex];
            LoadState(saveData);
            InputManager inputManager = XrdModule::GetInputManager();
            inputManager.SetP1ReplayPos(saveData.p1ReplayPosition);
            inputManager.SetP2ReplayPos(saveData.p2ReplayPosition);

            mCurrentFrame = availableIndex;
        }
    }

    // Simulate forward to desired frame.
    if (mCurrentFrame != index)
    {
        DisableSoundEffects();

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
        LPVOID gameManager = (LPVOID)XrdModule::GetEngine().GetGameLogicManager().GetPtr();
        while (mCurrentFrame != index)
        {
            // TODO: prevent HUD elements being re-rendered on each re-simulation.

            tickGame(gameManager, 1);

            // TODO: pretty sure actor ticks are supposed to happen before engine
            // ticks but it depends if we loaded before this ticking or not.
            tickRelevantActors();

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
                if (entity.GetComplexActor())
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

        ReEnableSoundEffects();
    }

    return mCurrentFrame;
}

size_t ReplayManager::LoadNextFrame()
{
    return LoadFrame(mCurrentFrame + 1);
}

size_t ReplayManager::LoadPreviousFrame()
{
    if (mCurrentFrame == 0)
    {
        return mCurrentFrame;
    }
    return LoadFrame(mCurrentFrame - 1);
}

size_t ReplayManager::GetCurrentFrame() const
{
    return mCurrentFrame;
}
