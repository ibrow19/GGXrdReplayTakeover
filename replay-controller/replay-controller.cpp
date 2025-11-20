#include <replay-controller.h>
#include <common.h>
#include <input.h>
#include <save-state.h>
#include <xrd-module.h>
#include <asw-engine.h>
#include <replay-hud.h>
#include <pause-menu.h>
#include <entity.h>
#include <imgui.h>
#include <detours.h>

class ReplayControllerDetourer
{
public:
    void DetourSetHealth(int newHealth);
    void DetourTickSimpleActor(float delta);
    void DetourDisplayReplayHudMenu();
    static SetHealthFunc mRealSetHealth; 
    static TickActorFunc mRealTickSimpleActor; 
    static DisplayReplayHudMenuFunc mRealDisplayReplayHudMenu; 
};
SetHealthFunc ReplayControllerDetourer::mRealSetHealth = nullptr;
TickActorFunc ReplayControllerDetourer::mRealTickSimpleActor = nullptr;
DisplayReplayHudMenuFunc ReplayControllerDetourer::mRealDisplayReplayHudMenu = nullptr;

static ReplayHudUpdateFunc GRealReplayHudUpdate = nullptr;
static AddUiTextFunc GRealAddUiText = nullptr;
static UpdateTimeFunc GRealUpdateTime = nullptr;
static HandleInputsFunc GRealHandleInputs = nullptr;
static bool GbReplayFrameStep = false;
static bool GbOverrideSimpleActorPause = false;
static bool GbOverrideHudText = false;
static bool GbAddingFirstTextRow = false;

void ReplayControllerDetourer::DetourSetHealth(int newHealth)
{
    ReplayController* controller = GameModeController::Get<ReplayController>();
    assert(controller);
    if (newHealth <= 0 && (controller->GetMode() == ReplayTakeoverMode::TakeoverControl))
    {
        controller->EndTakeoverRound();
        newHealth = 1;
    }
    mRealSetHealth(this, newHealth);
}

// We don't want to completely pause the game in replays as then it won't
// update with our loaded states. Instead we just prevent the Asw engine from
// updating and here make sure simple actors for things like projectiles don't
// progress their animations. There are probably better mechanisms for doing
// this somewhere in the game but I haven't found them and this works for now.
void ReplayControllerDetourer::DetourTickSimpleActor(float delta)
{
    ReplayController* controller = GameModeController::Get<ReplayController>();
    assert(controller);
    if (GbOverrideSimpleActorPause || !controller->IsPaused())
    {
        mRealTickSimpleActor(this, delta);
    }
    else
    {
        TimeStepData timeStepData = SimpleActor((DWORD)this).GetTimeStepData();
        if (timeStepData.IsValid() && timeStepData.ShouldUseFixedTimeStep())
        {
            float& fixedTimeStep = timeStepData.GetFixedTimeStep();
            float oldTimeStep = fixedTimeStep;
            fixedTimeStep = 0;
            mRealTickSimpleActor(this, 0);
            fixedTimeStep = oldTimeStep;
        }
        else
        {
            mRealTickSimpleActor(this, 0);
        }
    }
}

void ReplayControllerDetourer::DetourDisplayReplayHudMenu()
{
    GbOverrideHudText = true;
    GbAddingFirstTextRow = true;

    ReplayController* controller = GameModeController::Get<ReplayController>();
    assert(controller);
    if (controller->GetMode() == ReplayTakeoverMode::Disabled)
    {
        mRealDisplayReplayHudMenu((LPVOID)this);
    }
    else
    {
        // When takeover controls are enabled, temporarily edit hud settings
        // so that all lines of text will be displayed.
        ReplayHud hud = XrdModule::GetEngine().GetReplayHud();
        DWORD& shouldPause = hud.GetPause();
        DWORD& specialCamera = hud.GetUseSpecialCamera();
        DWORD& cameraUnavailable = hud.GetCameraUnavailable();
        DWORD realShouldPause = shouldPause;
        DWORD realSpecialCamera = specialCamera;
        DWORD realCameraUnavailable = cameraUnavailable;
        shouldPause = 1;
        specialCamera = 0;
        cameraUnavailable = 0;

        mRealDisplayReplayHudMenu((LPVOID)this);

        shouldPause = realShouldPause;
        specialCamera = realSpecialCamera;
        cameraUnavailable = realCameraUnavailable;
    }
    GbOverrideHudText = false;
}

void DetourAddUiText(DWORD* textParams, DWORD param1, DWORD param2, DWORD param3, DWORD param4, DWORD param5)
{
    GRealAddUiText(textParams, param1, param2, param3, param4, param5);
    if (GbOverrideHudText)
    {
        if (GbAddingFirstTextRow)
        {
            float* spacing = &(float)textParams[3];
            float prevSpacing = *spacing;

            // Play/pause isn't shown before round begins/after it ends so we
            // need less spacing in those cases.
            int textElements = 9;
            ReplayController* controller = GameModeController::Get<ReplayController>();
            assert(controller);
            if (!XrdModule::CheckInBattle())
            {
                --textElements;
            }
            *spacing += XrdModule::GetReplayTextSpacing() * textElements;

            // This entry in the text parameters is a label for identifying
            // the text to display. It is not the actual text that gets displayed.
            char** textLabel = &(char*)textParams[10];
            *textLabel = "TrainingEtc_ComboDamage";

            GRealAddUiText(textParams, param1, param2, param3, param4, param5);
            *spacing = prevSpacing;
            GbAddingFirstTextRow = false;
        }
    }
}

void __fastcall DetourReplayHudUpdate(DWORD param)
{
    if (GameModeController::Get<ReplayController>()->GetMode() == ReplayTakeoverMode::Disabled)
    {
        GRealReplayHudUpdate(param);
        if (XrdModule::GetEngine().GetReplayHud().GetShouldStepNextFrame())
        {
            GbReplayFrameStep = true;
        }
    }
}

void __fastcall DetourUpdateTime(DWORD timeData)
{
    GRealUpdateTime(timeData);
    int& time = *(int*)(timeData + 0xc);
    ReplayController* controller = GameModeController::Get<ReplayController>();
    assert(controller);
    if (time <= 0 && (controller->GetMode() == ReplayTakeoverMode::TakeoverControl))
    {
        controller->EndTakeoverRound();
        time = 1;
    }
}

void __fastcall DetourHandleInputs(DWORD engine)
{
    // Suppress errors from overriding inputs in replay.
    GRealHandleInputs(engine);
    XrdModule::GetEngine().GetErrorCode() = 0;
}

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
{
    GbReplayFrameStep = false;
}

void ReplayController::InitMode()
{
    // If we set player 2 to player control then it will look for input
    // from a second controller. However, we want it to use the first
    // controller. So we change this instruction in the input handling
    // so that it pushes player 1 index onto the stack instead of player 2 when
    // it's about to call the function for getting controller inputs it's going
    // to add to the input buffer. Normally this instruction is Push ESI which
    // is 0 or 1. We replace it with Push EDX as we know (pretty sure) that EDX
    // is always 0 here in replays. However, EDX can be 1 in other modes such
    // as training so we need to make sure we change the instruction back once
    // we're done so that we don't break the other game modes.
    BYTE* instruction = XrdModule::GetControllerIndexInstruction();
    MakeRegionWritable((DWORD)instruction, 1);
    *instruction = 0x52;

    InitPauseMenuMods();
    InitUiStrings();
    UpdateUi();
    AttachSaveStateDetours();

    GRealReplayHudUpdate = XrdModule::GetReplayHudUpdate();
    GRealAddUiText = XrdModule::GetAddUiText();
    GRealUpdateTime = XrdModule::GetUpdateTime();
    GRealHandleInputs = XrdModule::GetHandleInputs();
    ReplayControllerDetourer::mRealSetHealth = XrdModule::GetSetHealth();
    ReplayControllerDetourer::mRealTickSimpleActor = XrdModule::GetTickSimpleActor();
    ReplayControllerDetourer::mRealDisplayReplayHudMenu = XrdModule::GetDisplayReplayHudMenu();
    void (ReplayControllerDetourer::* detourSetHealth)(int) = &ReplayControllerDetourer::DetourSetHealth;
    void (ReplayControllerDetourer::* detourTickSimpleActor)(float) = &ReplayControllerDetourer::DetourTickSimpleActor;
    void (ReplayControllerDetourer::* detourDisplayReplayHudMenu)(void) = &ReplayControllerDetourer::DetourDisplayReplayHudMenu;

    DetourTransactionBegin();
    DetourUpdateThread(GetCurrentThread());
    DetourAttach(&(PVOID&)GRealReplayHudUpdate, DetourReplayHudUpdate);
    DetourAttach(&(PVOID&)GRealAddUiText, DetourAddUiText);
    DetourAttach(&(PVOID&)GRealUpdateTime, DetourUpdateTime);
    DetourAttach(&(PVOID&)GRealHandleInputs, DetourHandleInputs);
    DetourAttach(&(PVOID&)ReplayControllerDetourer::mRealSetHealth, *(PBYTE*)&detourSetHealth);
    DetourAttach(&(PVOID&)ReplayControllerDetourer::mRealTickSimpleActor, *(PBYTE*)&detourTickSimpleActor);
    DetourAttach(&(PVOID&)ReplayControllerDetourer::mRealDisplayReplayHudMenu, *(PBYTE*)&detourDisplayReplayHudMenu);
    DetourTransactionCommit();
}

void ReplayController::ShutdownMode()
{
    // Restore instruction to it's original value so it works in other game modes
    BYTE* instruction = XrdModule::GetControllerIndexInstruction();
    *instruction = 0x56;

    RestorePauseMenuSettings();
    RestoreUiStrings();
    DetachSaveStateDetours();

    void (ReplayControllerDetourer::* detourSetHealth)(int) = &ReplayControllerDetourer::DetourSetHealth;
    void (ReplayControllerDetourer::* detourTickSimpleActor)(float) = &ReplayControllerDetourer::DetourTickSimpleActor;
    void (ReplayControllerDetourer::* detourDisplayReplayHudMenu)(void) = &ReplayControllerDetourer::DetourDisplayReplayHudMenu;

    DetourTransactionBegin();
    DetourUpdateThread(GetCurrentThread());
    DetourDetach(&(PVOID&)GRealReplayHudUpdate, DetourReplayHudUpdate);
    DetourDetach(&(PVOID&)GRealAddUiText, DetourAddUiText);
    DetourDetach(&(PVOID&)GRealUpdateTime, DetourUpdateTime);
    DetourDetach(&(PVOID&)GRealHandleInputs, DetourHandleInputs);
    DetourDetach(&(PVOID&)ReplayControllerDetourer::mRealSetHealth, *(PBYTE*)&detourSetHealth);
    DetourDetach(&(PVOID&)ReplayControllerDetourer::mRealTickSimpleActor, *(PBYTE*)&detourTickSimpleActor);
    DetourDetach(&(PVOID&)ReplayControllerDetourer::mRealDisplayReplayHudMenu, *(PBYTE*)&detourDisplayReplayHudMenu);
    DetourTransactionCommit();
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

    if (!mReplayManager.IsEmpty())
    {
        int selectedFrame = mReplayManager.GetCurrentFrame();
        ImGui::SliderInt("Replay Frame", &selectedFrame, 0, mReplayManager.GetFrameCount() - 1);
    }
    ImGui::End();
}

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
        mBookmarkFrame = mReplayManager.GetCurrentFrame();
        mCountdown = 0;
        mMode = ReplayTakeoverMode::TakeoverCountdown;
        return;
    }

    // Replay scrubbing while paused.
    if (mMode == ReplayTakeoverMode::StandbyPaused)
    {
        GbOverrideSimpleActorPause = true;
        if ((battleHeld & (DWORD)BattleInputMask::Left) ||
            (battlePressed & (DWORD)BattleInputMask::S))
        {
            mReplayManager.LoadPreviousFrame();
        }
        else if ((battleHeld & (DWORD)BattleInputMask::Right) ||
                 (battlePressed & (DWORD)BattleInputMask::H))
        {
            mReplayManager.LoadNextFrame();
        }
        else if (battleHeld & (DWORD)BattleInputMask::Down)
        {
            size_t currentFrame = mReplayManager.GetCurrentFrame();
            if (currentFrame <= PausedFrameJump)
            {
                mReplayManager.LoadFrame(0);
            }
            else
            {
                mReplayManager.LoadFrame(currentFrame - PausedFrameJump);
            }
        }
        else if (battleHeld & (DWORD)BattleInputMask::Up)
        {
            size_t newFrame = mReplayManager.GetCurrentFrame() + PausedFrameJump;
            size_t maxFrame = mReplayManager.GetFrameCount() - 1;
            if (newFrame > maxFrame)
            {
                newFrame = maxFrame;
            }
            mReplayManager.LoadFrame(newFrame);
        }
        GbOverrideSimpleActorPause = false;
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
}

void ReplayController::HandleTakeoverMode()
{
    GameInputCollection input = XrdModule::GetGameInput();
    DWORD& battleInput = input.GetP1BattleInput().GetPressedMask();
    int countdownTotal = XrdModule::GetTrainingP1MaxHealth();

    // Restart takeover
    if (battleInput & (DWORD)BattleInputMask::PlayRecording)
    {
        GbOverrideSimpleActorPause = true;
        mReplayManager.LoadFrame(mBookmarkFrame);
        GbOverrideSimpleActorPause = false;
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
        GbOverrideSimpleActorPause = true;
        mReplayManager.LoadFrame(mBookmarkFrame);
        GbOverrideSimpleActorPause = false;
        ResetPlayerControl();
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
        GbReplayFrameStep = false;
        mReplayManager.Reset();
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
         GbReplayFrameStep)
    {
        mReplayManager.RecordFrame();
        GbReplayFrameStep = false;
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
        DWORD* pause = XrdModule::GetEngine().GetPauseEngineUpdateFlag();
        if (pause != nullptr)
        {
            *pause = 1;
        }
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
