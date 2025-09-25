#pragma once

#include <windows.h>
#include <d3d9.h>
#include <xrd-module.h>
#include <replay-manager.h>

enum class ReplayTakeoverMode
{
    Disabled,
    Standby,
    StandbyPaused,
    TakeoverCountdown,
    TakeoverControl,
};

class ReplayController
{
public:
    static void CreateInstance();
    static void DestroyInstance();
    static bool IsActive();
    static ReplayController& Get();
    static void InitRealPresent();

    void RenderUi(IDirect3DDevice9* device);
    void Tick();
    bool IsInReplayTakeoverMode() const;
private:
    ReplayController();
    ~ReplayController();

    static void AttachDetours();
    static void DetachDetours();

    void InitImGui(IDirect3DDevice9* device);
    void ShutdownImGui();
    void PrepareImGuiFrame();

    void OverridePlayerControl();
    void ResetPlayerControl();

    void HandleDisabledMode();
    void HandleStandbyMode();
    void HandleTakeoverMode();
private:
    static constexpr int MinCountdown = 0;
    static constexpr int MaxCountdown = 60;
    static constexpr int DefaultCountdown = 10;
    static constexpr size_t PausedFrameJump = 10;
private:
    static ReplayController* mInstance;
    static bool mbImGuiInitialised;

    ReplayTakeoverMode mMode;
    bool mbControlP1;
    int mCountdownTotal;
    int mCountdown;
    size_t mBookmarkFrame;
    ReplayManager mReplayManager;
};
