#pragma once

#include <windows.h>

enum class PauseMenuButton : DWORD
{
    Sample,
    ResetPosition,
    SelectTrial,
    ButtonDisplay,
    DisplayInputs1,
    DisplayInputs2,
    JpHint,
    Restart,
    FightingScreenChat,
    P1Character,
    P2Character,
    Faq,
    Unknown,
    CommandList,
    ButtonSettings,
    RestoreDefaults,
    DisplaySettings,
    DisplayComboRecipe,
    HpRegen,
    P1MaxHealth,
    P2MaxHealth,
    TensionGauge,
    PsychBurst,
    RiscLevel,
    CounterHit,
    DangerTime,
    Stun,
    EnemyStatus,
    ComLevel,
    RecPlay,
    Stance,
    BlockSwitching,
    BlockType,
    Recovery,
    StaggerRecovery,
    StunRecovery,
    AutoPsychBurst,
    P1InputLatency,
    P2InputLatency,
    CharacterSpecificSettings,
    JpPositionReset,
    Empty,
    CommandList2,
    JpStance,
    ComCharacter,
    AiDifficulty,
    WelcomeChallenger,
    DisplayCombo,
    CameraHorizontalAxis,
    CameraVerticalAxis,
    DisplayInstructionManual,
    QuickSnapshot,
    Save,
    Load,
    BgmSelection,
    SoundSettings,
    HideMenu,
    Unknown2,
    CharacterSelect,
    ReturnToReplaySelection,
    MainMenu,
    Retire,
    SpectatorList,
    ChatDuringBattle,
    DisplayInputs3,
    ShowDamageInformation,
    ReturnToRoom,
    TitleScreen,
    PSM_ACTrialEnd,
    Close,
    Count
};

enum class PauseMenuMode : DWORD
{
    Replay = 8,
    Count = 16
};

// Table of which pause menu buttons are enabled in which game mode.
class PauseMenuButtonTable
{
public:
    static constexpr size_t Size = (DWORD)PauseMenuButton::Count * (DWORD)PauseMenuMode::Count;
public:
    PauseMenuButtonTable(DWORD inPtr);
    DWORD GetPtr() const;
    BYTE& GetFlag(PauseMenuMode mode, PauseMenuButton button);
private:
    DWORD mPtr;
};
