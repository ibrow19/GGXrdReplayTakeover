#pragma once

#include <replay-record.h>
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
// is built associating labels with a pointer to a string to display. This
// struct stores info required to change to pointer in that table and later
// restore it to its original value. Pretty sure this is UE3's equivalent of
// FText, the string pointer gets changed depending on the language so if we
// change the pointer our UI overriding will happen regardless of language selected.
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

struct ReplayPauseMenuSettings
{
    DWORD p1MaxHealth = 0;
    DWORD buttonDisplayMode = 0;
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
#ifdef USE_IMGUI_OVERLAY
    void PrepareImGuiFrame() override;
#endif

    void InitUiStrings();
    void RestoreUiStrings();
    void UpdateUi();

    void InitPauseMenuMods();
    void RestorePauseMenuSettings();

    void OverridePlayerControl();
    void ResetPlayerControl();

    void HandleDisabledMode();
    void HandleStandbyMode();
    void HandleTakeoverMode();
private:
    static constexpr int DefaultCountdown = 15;
    static constexpr size_t PausedFrameJump = 3;
private:
    ReplayTakeoverMode mMode;
    int mCountdown;
    size_t mBookmarkFrame;
    ReplayRecord mRecord;
    ReplayUiStrings mUiStrings;
    ReplayPauseMenuSettings mOldPauseSettings;
};
