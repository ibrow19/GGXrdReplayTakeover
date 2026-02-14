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
    static constexpr size_t CharCodeLen = 32;
private:
    SaveData mSaveData;
    char mP1Char[CharCodeLen];
    char mP2Char[CharCodeLen];
    DWORD mEngineAddress = 0;
    bool mbValidSaveData = false;
    bool mbResetMode = false;
};
