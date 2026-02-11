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
#include <implot.h>
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
: mbPauseForOneFrame(false)
, mMode(ReplayTakeoverMode::Standby)
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

    auto showMode = [](const char* text, ImU32 colour)
        {
            ImGui::PushStyleColor(ImGuiCol_Text, colour);
            ImGui::Text(text);
            ImGui::PopStyleColor();
        };

    switch (mMode)
    {
        case ReplayTakeoverMode::Disabled:
            showMode("DISABLED", IM_COL32(255, 0, 0, 255));
            break;
        case ReplayTakeoverMode::StandbyPaused:
        case ReplayTakeoverMode::Standby:
            showMode("STANDBY", IM_COL32(200, 200, 0, 255));
            break;
        case ReplayTakeoverMode::TakeoverCountdown:
            showMode("COUNTDOWN", IM_COL32(0, 255, 100, 255));
            break;
        case ReplayTakeoverMode::TakeoverControl:
            showMode("TAKEOVER", IM_COL32(0, 255, 0, 255));
            break;
        case ReplayTakeoverMode::TakeoverRoundEnded:
            showMode("ROUND ENDED", IM_COL32(200, 200, 0, 255));
            break;
    }

    int playerNum = XrdModule::GetButtonDisplayMode() + 1;
    ImGui::Text("Control: Player %d", playerNum);

#if PROFILING
    STAT_GRAPH(ReplayRecord_MappingView)
    STAT_GRAPH(SaveState)
    STAT_GRAPH(LoadState)
    STAT_GRAPH(ReplayRecord_SetFrame)
    STAT_GRAPH(MainLogic)
#endif

    ImGui::End();
}
#endif

void ReplayController::ResetPlayerControl()
{
    InputManager inputManager = XrdModule::GetInputManager();
    inputManager.SetP1InputMode(InputMode::Replay);
    inputManager.SetP2InputMode(InputMode::Replay);
}

void ReplayController::InitiateCountdown()
{
    // Override player control based on current settings.
    // (Player control is stored in repurposed button display mode)
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

    mCountdown = 0;

    // Note: by always going to countdown mode and not straight to takeover mode
    // when the countdown total is 0 we force at least 1 frame of countdown.
    mMode = ReplayTakeoverMode::TakeoverCountdown;
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
        mBookmarkFrame = mRecord.GetCurrentFrame();
        InitiateCountdown();
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

    auto fastRewind = [this]
        {
            size_t currentFrame = this->mRecord.GetCurrentFrame();
            if (currentFrame <= PausedFrameJump)
            {
                this->mRecord.SetFrame(0);
            }
            else
            {
                this->mRecord.SetFrame(currentFrame - PausedFrameJump);
            }
        };

    auto fastForward = [this]
        {
            size_t newFrame = mRecord.GetCurrentFrame() + PausedFrameJump;
            mRecord.SetFrame(newFrame);
        };

    // Replay scrubbing.
    // While paused allow any navigation
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
            fastRewind();
        }
        else if (battleHeld & (DWORD)BattleInputMask::Up)
        {
            fastForward();
        }
    }
    // While unpaused only allow rewinding, fast forward, fast rewind
    // And force the next frame to be paused so that we don't advance
    // back to the same frame before the next tick.
    else
    {
        if (battleHeld & (DWORD)BattleInputMask::Left)
        {
            mRecord.SetPreviousFrame();
            mbPauseForOneFrame = true;
        }
        else if (battleHeld & (DWORD)BattleInputMask::Down)
        {
            fastRewind();
            mbPauseForOneFrame = true;
        }
        else if (battleHeld & (DWORD)BattleInputMask::Up)
        {
            fastForward();
            mbPauseForOneFrame = true;
        }
    }
}

void ReplayController::HandleTakeoverMode()
{
    GameInputCollection input = XrdModule::GetGameInput();
    DWORD& battleInput = input.GetP1BattleInput().GetPressedMask();

    // Restart takeover
    if (battleInput & (DWORD)BattleInputMask::PlayRecording)
    {
        // Need to reset player control first in case loading the bookmark frame
        // involves any resimulation.
        ResetPlayerControl();
        mRecord.SetFrame(mBookmarkFrame, /*bForceLoad=*/true);
        InitiateCountdown();
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
        // Countdown total is stored in repurposed training mode P1 max health.
        int countdownTotal = XrdModule::GetTrainingP1MaxHealth();
        if (mCountdown >= countdownTotal)
        {
            mMode = ReplayTakeoverMode::TakeoverControl;
        }
    }
}

void ReplayController::Tick()
{
    // Work around for burst max sounds getting queued up in resimulation.
    // We disable the burst sounds if the game is paused and make sure they 
    // are re-enabled each tick. A better solution would be possible with
    // a better understanding of how system sounds get played.
    ReplayDetourSettings::bDisableBurstMaxSound = false;

    // Re-enable sound effects if they got disabled by the mod on a previous frame.
    EnableSoundEffects();

    if (XrdModule::IsPauseMenuActive())
    {
        return;
    }

    // Reset takeover settings when round begins/ends.
    if (XrdModule::GetPreOrPostBattle())
    {
        mbPauseForOneFrame = false;
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

    // Record frame when in a state where the game is advancing.
    ReplayHud hud = XrdModule::GetEngine().GetReplayHud();
    if ((mMode == ReplayTakeoverMode::Disabled && !hud.GetPause()) || 
         (mMode == ReplayTakeoverMode::Standby && !mbPauseForOneFrame) ||
         ReplayDetourSettings::bReplayFrameStep)
    {
        mRecord.RecordFrame();
        ReplayDetourSettings::bReplayFrameStep = false;
    }

    mbPauseForOneFrame = false;

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
        ReplayDetourSettings::bDisableBurstMaxSound = true;
        DisableSoundEffects();
    }
}

ReplayTakeoverMode ReplayController::GetMode() const
{
    return mMode;
}

bool ReplayController::IsPaused() const
{
    return mbPauseForOneFrame ||
           mMode == ReplayTakeoverMode::StandbyPaused || 
           mMode == ReplayTakeoverMode::TakeoverCountdown ||
           mMode == ReplayTakeoverMode::TakeoverRoundEnded;
}

void ReplayController::EndTakeoverRound()
{
    assert(mMode == ReplayTakeoverMode::TakeoverControl);
    mMode = ReplayTakeoverMode::TakeoverRoundEnded;
}
