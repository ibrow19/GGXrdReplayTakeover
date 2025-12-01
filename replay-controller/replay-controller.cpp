#include <replay-controller.h>
#include <replay-mods.h>
#include <input.h>
#include <common.h>
#include <xrd-module.h>
#include <asw-engine.h>
#include <replay-hud.h>
#include <pause-menu.h>
#include <cassert>

#ifdef USE_IMGUI_OVERLAY
#include <imgui.h>
#endif

UiString::UiString()
: mPtr(nullptr)
, mOriginal(nullptr)
{}

UiString::UiString(DWORD inPtr)
: mPtr((char16_t**)inPtr)
, mOriginal(*mPtr)
{}

void UiString::Restore()
{
    *mPtr = mOriginal;
}

void UiString::Clear()
{
    *mPtr = u"";
}

void UiString::Set(char16_t* newString)
{
    *mPtr = newString;
}

ReplayController::ReplayController()
: mMode(ReplayTakeoverMode::Standby)
, mCountdown(0)
, mBookmarkFrame(0)
{}

void ReplayController::InitMode()
{
    AddReplayMods();
    InitPauseMenuMods();
    InitUiStrings();
    UpdateUi();
}

void ReplayController::ShutdownMode()
{
    RemoveReplayMods();
    RestorePauseMenuSettings();
    RestoreUiStrings();
}

void ReplayController::InitUiStrings()
{
    DWORD table = XrdModule::GetUiStringTable();

    mUiStrings.play = UiString(table + 0x54ecc);
    mUiStrings.toggleHud = UiString(table + 0x54f04);
    mUiStrings.frameStep = UiString(table + 0x54f20);
    mUiStrings.inputHistory = UiString(table + 0x5906c);
    mUiStrings.nextRound = UiString(table + 0x54f58);
    mUiStrings.pauseMenu = UiString(table + 0x54f74);
    mUiStrings.toggleControl = UiString(table + 0x54f90);
    mUiStrings.toggleCamera = UiString(table + 0x54fac);
    mUiStrings.buttonDisplay = UiString(table + 0x48b20);
    mUiStrings.buttonDisplayMode1 = UiString(table + 0x48b3c);
    mUiStrings.buttonDisplayMode2 = UiString(table + 0x48b58);
    mUiStrings.p1MaxHealth = UiString(table + 0x48e30);
    mUiStrings.comboDamage = UiString(table + 0x4a480);

    // Overwrite some pause menu text immediately, these will be the same regardless of takevoer state.
    mUiStrings.buttonDisplay.Set(u"Takeover Player");
    mUiStrings.buttonDisplayMode1.Set(u"Player 1");
    mUiStrings.buttonDisplayMode2.Set(u"Player 2");
    mUiStrings.p1MaxHealth.Set(u"Takeover Countdown Frames");
}

void ReplayController::RestoreUiStrings()
{
    mUiStrings.play.Restore();
    mUiStrings.toggleHud.Restore();
    mUiStrings.frameStep.Restore();
    mUiStrings.inputHistory.Restore();
    mUiStrings.nextRound.Restore();
    mUiStrings.pauseMenu.Restore();
    mUiStrings.toggleControl.Restore();
    mUiStrings.toggleCamera.Restore();
    mUiStrings.buttonDisplay.Restore();
    mUiStrings.buttonDisplayMode1.Restore();
    mUiStrings.buttonDisplayMode2.Restore();
    mUiStrings.p1MaxHealth.Restore();
    mUiStrings.comboDamage.Restore();
}

void ReplayController::UpdateUi()
{
    switch (mMode)
    {
        case ReplayTakeoverMode::Disabled:
            mUiStrings.play.Restore();
            mUiStrings.toggleHud.Restore();
            mUiStrings.frameStep.Restore();
            mUiStrings.inputHistory.Restore();
            mUiStrings.nextRound.Restore();
            mUiStrings.pauseMenu.Restore();
            mUiStrings.toggleControl.Restore();
            mUiStrings.toggleCamera.Restore();
            mUiStrings.comboDamage.Set(u"^mBtnSelect;: Replay Takeover Controls");
            break;
        case ReplayTakeoverMode::StandbyPaused:
        case ReplayTakeoverMode::Standby:
            if (mMode == ReplayTakeoverMode::Standby)
            {
                mUiStrings.play.Set(u"Mode: Replay");
                mUiStrings.toggleHud.Set(u"^mAtkP;: Pause");
            }
            else
            {
                mUiStrings.play.Set(u"Mode: Paused");
                mUiStrings.toggleHud.Set(u"^mAtkP;: Play");
            }
            mUiStrings.frameStep.Set(u"^mAtkPlay;: Takeover");
            mUiStrings.inputHistory.Set(u"^mAtkRec;: Cancel Takeover");
            mUiStrings.nextRound.Set(u"^mBtnLR;: Forward/Rewind");
            mUiStrings.pauseMenu.Set(u"^mBtnUD;: Fast Forward/Rewind");
            mUiStrings.toggleControl.Set(u"^mAtkS;/^mAtkH;: Frame Step");
            mUiStrings.toggleCamera.Set(u"^mAtkD;: Toggles Control Display");
            mUiStrings.comboDamage.Set(u"^mBtnSelect;: Standard Replay Controls");
            break;
        case ReplayTakeoverMode::TakeoverCountdown:
        case ReplayTakeoverMode::TakeoverControl:
        case ReplayTakeoverMode::TakeoverRoundEnded:
            if (mMode == ReplayTakeoverMode::TakeoverCountdown)
            {
                mUiStrings.play.Set(u"Mode: Countdown");
            }
            else if (mMode == ReplayTakeoverMode::TakeoverControl)
            {
                mUiStrings.play.Set(u"Mode: Takeover");
            }
            else
            {
                mUiStrings.play.Set(u"Mode: Round Ended");
            }
            mUiStrings.toggleHud.Clear();
            mUiStrings.frameStep.Set(u"^mAtkPlay;: Restart Takeover");
            mUiStrings.inputHistory.Set(u"^mAtkRec;: Cancel Takeover");
            mUiStrings.nextRound.Clear();
            mUiStrings.pauseMenu.Clear();
            mUiStrings.toggleControl.Clear();
            mUiStrings.toggleCamera.Clear();
            mUiStrings.comboDamage.Clear();
            break;
    }
}

void ReplayController::InitPauseMenuMods()
{
    PauseMenuButtonTable table = XrdModule::GetPauseMenuButtonTable();
    MakeRegionWritable(table.GetPtr(), (size_t)PauseMenuButtonTable::Size);
    table.GetFlag(PauseMenuMode::Replay, PauseMenuButton::ButtonDisplay) = 1;
    table.GetFlag(PauseMenuMode::Replay, PauseMenuButton::P1MaxHealth) = 1;
    DWORD& p1MaxHealth = XrdModule::GetTrainingP1MaxHealth();
    DWORD& buttonDisplay = XrdModule::GetButtonDisplayMode();
    mOldPauseSettings.p1MaxHealth = p1MaxHealth;
    mOldPauseSettings.buttonDisplayMode = buttonDisplay;
    p1MaxHealth = DefaultCountdown;
    buttonDisplay = 0;
}

void ReplayController::RestorePauseMenuSettings()
{
    PauseMenuButtonTable table = XrdModule::GetPauseMenuButtonTable();
    table.GetFlag(PauseMenuMode::Replay, PauseMenuButton::ButtonDisplay) = 0;
    table.GetFlag(PauseMenuMode::Replay, PauseMenuButton::P1MaxHealth) = 0;
    XrdModule::GetTrainingP1MaxHealth() = mOldPauseSettings.p1MaxHealth;
    XrdModule::GetButtonDisplayMode() = mOldPauseSettings.buttonDisplayMode;
}

#ifdef USE_IMGUI_OVERLAY
void ReplayController::PrepareImGuiFrame()
{
    ImGui::Begin("Replay Takeover");

    ImGui::Text("Takeover Mode: ");
    ImGui::SameLine();
    switch (mMode)
    {
        case ReplayTakeoverMode::Disabled:
            ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(255, 0, 0, 255));
            ImGui::Text("DISABLED");
            ImGui::PopStyleColor();
            break;
        case ReplayTakeoverMode::StandbyPaused:
        case ReplayTakeoverMode::Standby:
            ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(200, 200, 0, 255));
            ImGui::Text("STANDBY");
            ImGui::PopStyleColor();
            break;
        case ReplayTakeoverMode::TakeoverCountdown:
            ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(0, 255, 100, 255));
            ImGui::Text("COUNTDOWN");
            ImGui::PopStyleColor();
            break;
        case ReplayTakeoverMode::TakeoverControl:
            ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(0, 255, 0, 255));
            ImGui::Text("TAKEOVER");
            ImGui::PopStyleColor();
            break;
        case ReplayTakeoverMode::TakeoverRoundEnded:
            ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(200, 200, 0, 255));
            ImGui::Text("ROUND ENDED");
            ImGui::PopStyleColor();
            break;
    }

    int playerNum = XrdModule::GetButtonDisplayMode() + 1;
    ImGui::Text("Control: Player %d", playerNum);

    ImGui::End();
}
#endif

void ReplayController::OverridePlayerControl()
{
    InputManager inputManager = XrdModule::GetInputManager();
    if (XrdModule::GetButtonDisplayMode())
    {
        inputManager.SetP1InputMode(InputMode::Replay);
        inputManager.SetP2InputMode(InputMode::Player);
    }
    else
    {
        inputManager.SetP1InputMode(InputMode::Player);
        inputManager.SetP2InputMode(InputMode::Replay);
    }
}

void ReplayController::ResetPlayerControl()
{
    InputManager inputManager = XrdModule::GetInputManager();
    inputManager.SetP1InputMode(InputMode::Replay);
    inputManager.SetP2InputMode(InputMode::Replay);
}

void ReplayController::HandleDisabledMode()
{
    GameInputCollection input = XrdModule::GetGameInput();
    DWORD& menuInput = input.GetP1MenuInput().GetPressedMask();

    // Disable normal replay controls, enable replay takeover
    if (menuInput & (DWORD)MenuInputMask::Reset)
    {
        ReplayHud hud = XrdModule::GetEngine().GetReplayHud();
        // TODO: make pause seamless on switching.
        hud.GetPause() = 0;
        hud.GetShouldStepNextFrame() = 0;
        menuInput = 0;
        mMode = ReplayTakeoverMode::Standby;
    }
}

void ReplayController::HandleStandbyMode()
{
    GameInputCollection input = XrdModule::GetGameInput();
    DWORD& menuPressed = input.GetP1MenuInput().GetPressedMask();
    DWORD& battlePressed = input.GetP1BattleInput().GetPressedMask();
    DWORD& battleHeld = input.GetP1BattleInput().GetHeldMask();

    // Return to regular replay mode
    if (menuPressed & (DWORD)MenuInputMask::Reset)
    {
        ResetPlayerControl();
        XrdModule::GetEngine().GetReplayHud().GetDisplayReplayHud() = 1;
        menuPressed = 0;
        mMode = ReplayTakeoverMode::Disabled;
        return;
    }

    // Initiate takeover
    if (battlePressed & (DWORD)BattleInputMask::PlayRecording)
    {
        OverridePlayerControl();
        mBookmarkFrame = mRecord.GetCurrentFrame();
        mCountdown = 0;
        mMode = ReplayTakeoverMode::TakeoverCountdown;
        return;
    }

    // Toggle control display
    if (battlePressed & (DWORD)BattleInputMask::D)
    {
        DWORD& displayControls = XrdModule::GetEngine().GetReplayHud().GetDisplayReplayHud();
        displayControls = !displayControls;
        battlePressed ^= (DWORD)BattleInputMask::D;
    }

    // Toggle pause
    if (battlePressed & (DWORD)BattleInputMask::P)
    {
        if (mMode == ReplayTakeoverMode::Standby)
        {
            mMode = ReplayTakeoverMode::StandbyPaused;
        }
        else
        {
            mMode = ReplayTakeoverMode::Standby;
        }
    }

    // Replay scrubbing.
    if (mMode == ReplayTakeoverMode::StandbyPaused)
    {
        if ((battleHeld & (DWORD)BattleInputMask::Left) ||
            (battlePressed & (DWORD)BattleInputMask::S))
        {
            mRecord.SetPreviousFrame();
        }
        else if ((battleHeld & (DWORD)BattleInputMask::Right) ||
                 (battlePressed & (DWORD)BattleInputMask::H))
        {
            mRecord.SetNextFrame();
        }
        else if (battleHeld & (DWORD)BattleInputMask::Down)
        {
            size_t currentFrame = mRecord.GetCurrentFrame();
            if (currentFrame <= PausedFrameJump)
            {
                mRecord.SetFrame(0);
            }
            else
            {
                mRecord.SetFrame(currentFrame - PausedFrameJump);
            }
        }
        else if (battleHeld & (DWORD)BattleInputMask::Up)
        {
            size_t newFrame = mRecord.GetCurrentFrame() + PausedFrameJump;
            mRecord.SetFrame(newFrame);
        }
    }
}

void ReplayController::HandleTakeoverMode()
{
    GameInputCollection input = XrdModule::GetGameInput();
    DWORD& battleInput = input.GetP1BattleInput().GetPressedMask();
    int countdownTotal = XrdModule::GetTrainingP1MaxHealth();

    // Restart takeover
    if (battleInput & (DWORD)BattleInputMask::PlayRecording)
    {
        // Need to reset player control first in case loading the bookmark frame
        // involves any resimulation.
        ResetPlayerControl();
        mRecord.SetFrame(mBookmarkFrame, /*bForceLoad=*/true);
        OverridePlayerControl();
        if (countdownTotal == 0)
        {
            mMode = ReplayTakeoverMode::TakeoverControl;
        }
        else
        {
            mCountdown = 0;
            mMode = ReplayTakeoverMode::TakeoverCountdown;
        }
        return;
    }

    // Cancel takeover
    if (battleInput & (DWORD)BattleInputMask::StartRecording)
    {
        // Make sure player control reset first so it doesn't interfere with
        // any potential resimulation when loading the bookmark.
        ResetPlayerControl();
        mRecord.SetFrame(mBookmarkFrame, /*bForceLoad=*/true);
        mMode = ReplayTakeoverMode::StandbyPaused;
        return;
    }

    // Progress countdown
    if (mMode == ReplayTakeoverMode::TakeoverCountdown)
    {
        ++mCountdown;
        if (mCountdown >= countdownTotal)
        {
            mMode = ReplayTakeoverMode::TakeoverControl;
        }
    }
}

void ReplayController::Tick()
{
    if (XrdModule::IsPauseMenuActive())
    {
        return;
    }

    if (XrdModule::GetPreOrPostBattle())
    {
        ReplayDetourSettings::bReplayFrameStep = false;
        mRecord.Reset();
        if (mMode != ReplayTakeoverMode::Standby)
        {
            mMode = ReplayTakeoverMode::Standby;
            UpdateUi();
            ResetPlayerControl();
        }
        return;
    }

    ReplayHud hud = XrdModule::GetEngine().GetReplayHud();
    if ((mMode == ReplayTakeoverMode::Disabled && !hud.GetPause()) || 
         mMode == ReplayTakeoverMode::Standby ||
         ReplayDetourSettings::bReplayFrameStep)
    {
        mRecord.RecordFrame();
        ReplayDetourSettings::bReplayFrameStep = false;
    }

    switch (mMode)
    {
        case ReplayTakeoverMode::Disabled:
            HandleDisabledMode();
            break;
        case ReplayTakeoverMode::Standby:
        case ReplayTakeoverMode::StandbyPaused:
            HandleStandbyMode();
            break;
        case ReplayTakeoverMode::TakeoverCountdown:
        case ReplayTakeoverMode::TakeoverControl:
        case ReplayTakeoverMode::TakeoverRoundEnded:
            HandleTakeoverMode();
            break;
    }

    UpdateUi();

    if (IsPaused())
    {
        XrdModule::GetEngine().GetGameLogicManager().GetPauseEngineUpdateFlag() = 1;
    }
}

ReplayTakeoverMode ReplayController::GetMode() const
{
    return mMode;
}

bool ReplayController::IsPaused() const
{
    return mMode == ReplayTakeoverMode::StandbyPaused || 
           mMode == ReplayTakeoverMode::TakeoverCountdown ||
           mMode == ReplayTakeoverMode::TakeoverRoundEnded;
}

void ReplayController::EndTakeoverRound()
{
    assert(mMode == ReplayTakeoverMode::TakeoverControl);
    mMode = ReplayTakeoverMode::TakeoverRoundEnded;
}
