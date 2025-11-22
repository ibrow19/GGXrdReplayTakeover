#pragma once

#include <save-state.h>
#include <game-mode-controller.h>

struct TrainingSaveData : public SaveData
{
    bool bValid = false;
};

class TrainingController : public GameModeController
{
private:
    void InitMode() override;
    void ShutdownMode() override;
    void Tick() override;
#ifdef USE_IMGUI_OVERLAY
    void PrepareImGuiFrame() override;
#endif
private:
#ifdef USE_IMGUI_OVERLAY
    static constexpr size_t SaveStateCount = 10;
#else
    static constexpr size_t SaveStateCount = 1;
#endif
private:
    size_t mSelectedState = 0; 
    DWORD mEnginePtr = 0;
    TrainingSaveData mSaveStates[SaveStateCount];
};
