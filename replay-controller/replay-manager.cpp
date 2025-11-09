#include <replay-manager.h>
#include <xrd-module.h>
#include <common.h>

ReplayManager::ReplayManager()
: mFrameCount(0),
  mStart(0),
  mCurrentFrame(0)
{}

void ReplayManager::Reset()
{
    mFrameCount = 0;
    mStart = 0;
    mCurrentFrame = 0;
}

size_t ReplayManager::GetCurrentFrameBufferPos() const
{
    return (mStart + mCurrentFrame) % MaxFrameCount;
}

size_t ReplayManager::RecordFrame()
{
    if (!IsEmpty())
    {
        ++mCurrentFrame;
    }

    if (mCurrentFrame == mFrameCount)
    {
        ReplaySaveData& saveData = mSavedFrames[GetCurrentFrameBufferPos()];
        SaveState(saveData);
        InputManager inputManager = XrdModule::GetInputManager();
        saveData.p1ReplayPosition = inputManager.GetP1ReplayPos();
        saveData.p2ReplayPosition = inputManager.GetP2ReplayPos();

        if (mFrameCount == MaxFrameCount)
        {
            ++mStart;
            --mCurrentFrame;
        }
        else
        {
            ++mFrameCount;
        }
    }

    return mCurrentFrame;
}

size_t ReplayManager::LoadFrame(size_t index)
{
    if (index >= mFrameCount)
    {
        return mCurrentFrame;
    }

    mCurrentFrame = index;
    ReplaySaveData& saveData = mSavedFrames[GetCurrentFrameBufferPos()];
    LoadState(saveData);
    InputManager inputManager = XrdModule::GetInputManager();
    inputManager.SetP1ReplayPos(saveData.p1ReplayPosition);
    inputManager.SetP2ReplayPos(saveData.p2ReplayPosition);
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

size_t ReplayManager::GetFrameCount() const
{
    return mFrameCount;
}

bool ReplayManager::IsEmpty() const
{
    return mFrameCount == 0;
}
