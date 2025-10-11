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
    void InitMode() override;
    void ShutdownMode() override;
    void Tick() override;
    void PrepareImGuiFrame() override;
private:
    static constexpr size_t SaveStateCount = 10;
private:
    size_t mSelectedState = 0; 
    TrainingSaveState mSaveStates[SaveStateCount];
    DWORD mEnginePtr = 0;
};
