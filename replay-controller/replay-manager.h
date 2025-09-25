#pragma once

#include <save-state.h>
#include <input-manager.h>

// Ring buffer of recent frames in a replay with the ability to set
// the current game state to any of the frames currently in the buffer.
class ReplayManager
{
public:
    static constexpr size_t MaxFrameCount = 450;
public:
    ReplayManager();
    void Reset();

    size_t GetCurrentFrame() const;
    size_t GetFrameCount() const;
    bool IsEmpty() const;

    // Records the current game state into the next position after currentFrame;
    // If we already have MaxFrameCount frames then the oldest frame is overidden.
    // If we already have the replay frame after currentFrame saved then calling this
    // will simply increment currentFrame.
    // returns currentFrame.
    size_t RecordFrame();

    // Load a state from the buffer and set currentFrame to that position.
    // Returns currentFrame.
    size_t LoadFrame(size_t index);
    size_t LoadNextFrame();
    size_t LoadPreviousFrame();
private:
    size_t GetCurrentFrameBufferPos() const;
private:
    // Number of currently saved frame states.
    size_t frameCount;

    // Start position in ring buffer.
    size_t start;

    // Index from start rather than index into frames buffer.
    size_t currentFrame;

    char frames[MaxFrameCount * SaveStateSize];
    ReplayPosition p1ReplayPositions[MaxFrameCount];
    ReplayPosition p2ReplayPositions[MaxFrameCount];
};
