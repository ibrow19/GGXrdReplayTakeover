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

// Text used in the Ui. The strings are allocated when the game boots and a table
// is built associating widget lables with a pointer to text to display. This
// struct stores info required to change to pointer in that table and later
// restore it to its original value.
class UiString
{
public:
    UiString();
    UiString(DWORD inPtr);
    void Restore();
    void Clear();
    void Set(char16_t* newString);
private:
    char16_t** mPtr = nullptr;
    char16_t* mOriginal = nullptr;
};

struct ReplayUiStrings
{
    UiString play;
    UiString toggleHud;
    UiString frameStep;
    UiString inputHistory;
    UiString nextRound;
    UiString pauseMenu;
    UiString toggleControl;
    UiString toggleCamera;
    UiString buttonDisplay;
    UiString buttonDisplayMode1;
    UiString buttonDisplayMode2;
    UiString p1MaxHealth;
    UiString comboDamage;
};

class ReplayController : public GameModeController
{
public:
    ReplayController();
    ReplayTakeoverMode GetMode() const;
    bool IsPaused() const;
    void EndTakeoverRound();
private:
    void InitMode() override;
    void ShutdownMode() override;
    void Tick() override;
    void PrepareImGuiFrame() override;

    void InitUiStrings();
    void RestoreUiStrings();
    void UpdateUi();

    void OverridePlayerControl();
    void ResetPlayerControl();

    void HandleDisabledMode();
    void HandleStandbyMode();
    void HandleTakeoverMode();
private:
    static constexpr int MinCountdown = 0;
    static constexpr int MaxCountdown = 60;
    static constexpr int DefaultCountdown = 15;
    static constexpr size_t PausedFrameJump = 3;
private:
    ReplayTakeoverMode mMode;
    bool mbControlP1;
    int mCountdownTotal;
    int mCountdown;
    size_t mBookmarkFrame;
    ReplayManager mReplayManager;
    ReplayUiStrings mUiStrings;
};
