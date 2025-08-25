#include <replay-manager.h>

ReplayManager::ReplayManager()
{
    Reset();
}

void ReplayManager::Reset()
{
    bRecording = false;
    bRecordingComplete = false;
    currentFrame = 0;
}

void ReplayManager::BeginRecording()
{
    Reset();
    bRecording = true;
}

void ReplayManager::RecordFrame()
{
    if (!bRecording)
    {
        return;
    }

    SaveState(SavedFrames + currentFrame * SaveStateSize);
    ++currentFrame;

    if (currentFrame == FrameCount)
    {
        bRecording = false;
        bRecordingComplete = true;
    }
}

size_t ReplayManager::SetFrame(size_t index)
{
    if (!bRecordingComplete || index >= FrameCount)
    {
        return currentFrame;
    }

    LoadState(SavedFrames + index * SaveStateSize);
    currentFrame = index;
    return currentFrame;
}

size_t ReplayManager::IncrementFrame()
{
    return SetFrame(currentFrame + 1);
}

size_t ReplayManager::DecrementFrame()
{
    return SetFrame(currentFrame - 1);
}

bool ReplayManager::IsRecording() const
{
    return bRecording;
}

bool ReplayManager::IsRecordingComplete() const
{
    return bRecordingComplete;
}
