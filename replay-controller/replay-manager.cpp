#include <replay-manager.h>
#include <common.h>

ReplayManager::ReplayManager()
: frameCount(0),
  start(0),
  currentFrame(0)
{}

void ReplayManager::Init()
{
    inputManager.Init();
}

void ReplayManager::Reset()
{
    frameCount = 0;
    start = 0;
    currentFrame = 0;
}

size_t ReplayManager::GetCurrentFrameBufferPos() const
{
    return (start + currentFrame) % MaxFrameCount;
}

size_t ReplayManager::RecordFrame()
{
    if (!IsEmpty())
    {
        ++currentFrame;
    }

    if (currentFrame == frameCount)
    {
        size_t bufferIndex = GetCurrentFrameBufferPos();
        SaveState(frames + bufferIndex * SaveStateSize);
        replayPositions[bufferIndex] = inputManager.GetReplayPosition();

        if (frameCount == MaxFrameCount)
        {
            ++start;
            --currentFrame;
        }
        else
        {
            ++frameCount;
        }
    }

    return currentFrame;
}

size_t ReplayManager::LoadFrame(size_t index)
{
    if (index >= frameCount)
    {
        return currentFrame;
    }

    currentFrame = index;
    size_t bufferIndex = GetCurrentFrameBufferPos();
    LoadState(frames + bufferIndex * SaveStateSize);
    inputManager.SetReplayPosition(replayPositions[bufferIndex]);
    return currentFrame;
}

size_t ReplayManager::LoadNextFrame()
{
    return LoadFrame(currentFrame + 1);
}

size_t ReplayManager::LoadPreviousFrame()
{
    if (currentFrame == 0)
    {
        return currentFrame;
    }
    return LoadFrame(currentFrame - 1);
}

size_t ReplayManager::GetCurrentFrame() const
{
    return currentFrame;
}

size_t ReplayManager::GetFrameCount() const
{
    return frameCount;
}

bool ReplayManager::IsEmpty() const
{
    return frameCount == 0;
}
