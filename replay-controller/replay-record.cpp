#include <replay-record.h>
#include <replay-mods.h>
#include <xrd-module.h>
#include <asw-engine.h>
#include <entity.h>
#include <input.h>
#include <common.h>
#include <cassert>
#include <stdexcept>

ReplayRecord::ReplayRecord()
: mRecordedFrames(0),
  mCurrentFrame(0)
{
	SYSTEM_INFO systemInfo;
	GetSystemInfo(&systemInfo);
    mAllocationGranularity = systemInfo.dwAllocationGranularity;
    mReplaySaveDataSizeRoundedUp = (sizeof ReplaySaveData + mAllocationGranularity - 1) & ~(mAllocationGranularity - 1);
    mSpacedBufferFileMapping = CreateFileMappingW(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0,
        mReplaySaveDataSizeRoundedUp * SpacedBufferSize,
        NULL);
    if (!mSpacedBufferFileMapping) {
        ShowErrorBox();
        throw std::runtime_error("Failed to create a file mapping to hold all of the save states.");
    }
}

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
            SpacedBufferFileMappingView saveData = GetSaveData(mCurrentFrame / SaveStateSpacing);
            SaveState(*saveData.Data());
            InputManager inputRecord = XrdModule::GetInputManager();
            saveData.Data()->p1ReplayPosition = inputRecord.GetP1ReplayPos();
            saveData.Data()->p2ReplayPosition = inputRecord.GetP2ReplayPos();
        }
    }

    return mCurrentFrame;
}

size_t ReplayRecord::SetFrame(size_t index, bool bForceLoad)
{
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
            SpacedBufferFileMappingView saveData = GetSaveData(availableSpacedIndex);
            ReplayDetourSettings::bOverrideSimpleActorPause = true;
            LoadState(*saveData.Data());
            ReplayDetourSettings::bOverrideSimpleActorPause = false;
            InputManager inputRecord = XrdModule::GetInputManager();
            inputRecord.SetP1ReplayPos(saveData.Data()->p1ReplayPosition);
            inputRecord.SetP2ReplayPos(saveData.Data()->p2ReplayPosition);
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
        LPVOID gameManager = (LPVOID)XrdModule::GetEngine().GetGameLogicManager().GetPtr();
        while (mCurrentFrame != index)
        {
            tickGame(gameManager, 1);
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

SpacedBufferFileMappingView ReplayRecord::GetSaveData(int index)
{
    unsigned long long addr = index * (unsigned long long)mReplaySaveDataSizeRoundedUp;
    ReplaySaveData* data = (ReplaySaveData*)MapViewOfFile(mSpacedBufferFileMapping, FILE_MAP_WRITE,
        (DWORD)(addr >> 32),
        (DWORD)(addr & 0xFFFFFFFF),
        sizeof ReplaySaveData);
    
    if (!data) {
        ShowErrorBox();
        throw std::runtime_error("Failed to MapViewOfFile to access (read or write) a save state.");
    }
    return SpacedBufferFileMappingView(data);
}

SpacedBufferFileMappingView::SpacedBufferFileMappingView(ReplaySaveData* data) : data(data)
{}

SpacedBufferFileMappingView::~SpacedBufferFileMappingView()
{
    if (data) {
        UnmapViewOfFile(data);
    }
}

ReplayRecord::~ReplayRecord()
{
    if (mSpacedBufferFileMapping) {
        CloseHandle(mSpacedBufferFileMapping);
    }
}
