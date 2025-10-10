#pragma once

#include <replay-manager.h>
#include <game-mode-controller.h>

enum class ReplayTakeoverMode
{
    Disabled,
    Standby,
    StandbyPaused,
    TakeoverCountdown,
    TakeoverControl,
    TakeoverRoundEnded,
};

class ReplayController : public GameModeController
{
public:
    ReplayController();
    ReplayTakeoverMode GetMode() const;
    void EndTakeoverRound();
private:
    void AttachModeDetours() override;
    void DetachModeDetours() override;
    void Tick() override;
    void PrepareImGuiFrame() override;

    void OverridePlayerControl();
    void ResetPlayerControl();

    void HandleDisabledMode();
    void HandleStandbyMode();
    void HandleTakeoverMode();
private:
    static constexpr int MinCountdown = 0;
    static constexpr int MaxCountdown = 60;
    static constexpr int DefaultCountdown = 30;
    static constexpr size_t PausedFrameJump = 3;
private:
    ReplayTakeoverMode mMode;
    bool mbControlP1;
    int mCountdownTotal;
    int mCountdown;
    size_t mBookmarkFrame;
    ReplayManager mReplayManager;
};
