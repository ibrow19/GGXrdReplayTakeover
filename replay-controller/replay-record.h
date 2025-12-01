#pragma once

#include <save-state.h>
#include <input-manager.h>

struct ReplaySaveData : public SaveData
{
    ReplayPosition p1ReplayPosition;
    ReplayPosition p2ReplayPosition;
};

// Container of save states and input data from throughout a replay.
// Stores a state every X frames to support restoring earlier states in the
// replay. It's unreasonable to save every single frame since the save states
// are ~2mb. So instead by saving a state every couple frames we can load
// an old state then re-simulate the game up to where we want to be. This saves
// memory at the cost of processing time. Online rollbacks resimulate up to ~12
// frames so as long as we keep X around 12 the re-simulation should be comparable
// to a big rollback. We all save the most recent frames in another buffer to avoid 
// needing to constantly re-simulate when scrubbing around the same area.
class ReplayRecord
{
public:
    ReplayRecord();
    void Reset();

    size_t GetCurrentFrame() const;

    // This class is responsible for saving/loading states but advancing of the
    // actual game state can happen externally. So we need to call this function to
    // tell it that the game has advanced a frame. It will only save states
    // at the specified spacing but still needs to increment its internal frame
    // counter to keep position in the replay.
    // returns currentFrame.
    size_t RecordFrame();

    // Sets the replay the specified frame in the record. If the requested 
    // frame is further forward than our most recently saved frame then we 
    // need to simulate up to it. For loading an older frame, we jump to the 
    // nearest saved frame and simulate forward to it.
    // If the record is already at the specified index then it will not change
    // the game state. bForceLoad can be used to override this behaviour.
    // returns currentFrame after the action.
    size_t SetFrame(size_t index, bool bForceLoad = false);
    size_t SetNextFrame();
    size_t SetPreviousFrame();
private:
    // Max frames assuming default of 99 second rounds. Not currently 
    // compatible with infinite length rounds but nobody seriously uses
    // that anyway. It's not supported online and I'm not sure you can even
    // save replays with infinite time limit.
    static constexpr size_t MaxRoundFrames = 5940;

    static constexpr size_t SaveStateSpacing = 15;
    static constexpr size_t SpacedBufferSize = MaxRoundFrames / SaveStateSpacing;
private:
    size_t mRecordedFrames;
    size_t mCurrentFrame;
    ReplaySaveData mSpacedBuffer[SpacedBufferSize];
};
