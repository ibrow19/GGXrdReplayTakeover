#pragma once

#include <save-state.h>

class ReplayManager
{
public:

    static constexpr size_t FrameCount = 300;

public:

    ReplayManager();

    void BeginRecording();
    void RecordFrame();

    size_t SetFrame(size_t index);
    size_t IncrementFrame();
    size_t DecrementFrame();

    bool IsRecording() const;
    bool IsRecordingComplete() const;

private:

    void Reset();

private:

    bool bRecording;
    bool bRecordingComplete;

    // While recording this is the next frame that needs saved.
    // While not recording this is the currently loaded frame.
    size_t currentFrame;

    char SavedFrames[FrameCount * SaveStateSize];
};
