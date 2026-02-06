#pragma once

#include <save-state.h>
#include <game-mode-controller.h>

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
    SaveData mSaveData;
    bool mbValidSaveData = false;
    bool mbResetMode = false;
};
