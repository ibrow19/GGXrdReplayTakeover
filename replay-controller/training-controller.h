#pragma once

#include <save-state.h>
#include <game-mode-controller.h>

struct TrainingSaveState
{
    bool bValid = false;
    char saveData[SaveStateSize];
};

class TrainingController : public GameModeController
{
private:
    void AttachModeDetours() override;
    void DetachModeDetours() override;
    void Tick() override;
    void PrepareImGuiFrame() override;
private:
    static constexpr size_t SaveStateCount = 10;
private:
    static size_t mSelectedState; 
    static TrainingSaveState mSaveStates[SaveStateCount];
};
