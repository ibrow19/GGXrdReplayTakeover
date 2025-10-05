#include <replay-controller.h>
#include <input.h>
#include <save-state.h>
#include <xrd-module.h>
#include <asw-engine.h>
#include <replay-hud.h>
#include <imgui.h>
#include <detours.h>

static ReplayHudUpdateFunc GRealReplayHudUpdate = nullptr;
static bool GbReplayFrameStep = false;

void __fastcall DetourReplayHudUpdate(DWORD param)
{
    if (!GameModeController::Get<ReplayController>()->IsInReplayTakeoverMode())
    {
        GRealReplayHudUpdate(param);
        if (XrdModule::GetEngine().GetReplayHud().GetShouldStepNextFrame())
        {
            GbReplayFrameStep = true;
        }
    }
}

ReplayController::ReplayController()
: mMode(ReplayTakeoverMode::Disabled)
, mbControlP1(true)
, mCountdownTotal(DefaultCountdown)
, mCountdown(0)
, mBookmarkFrame(0)
{}

void ReplayController::AttachModeDetours()
{
    AttachSaveStateDetours();

    GRealReplayHudUpdate = XrdModule::GetReplayHudUpdate();

    DetourTransactionBegin();
    DetourUpdateThread(GetCurrentThread());
    DetourAttach(&(PVOID&)GRealReplayHudUpdate, DetourReplayHudUpdate);
    DetourTransactionCommit();
}

void ReplayController::DetachModeDetours()
{
    DetachSaveStateDetours();

    DetourTransactionBegin();
    DetourUpdateThread(GetCurrentThread());
    DetourDetach(&(PVOID&)GRealReplayHudUpdate, DetourReplayHudUpdate);
    DetourTransactionCommit();
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

    }

    int playerNum = mbControlP1 ? 1 : 2;
    ImGui::Text("Control: Player %d", playerNum);

    ImGui::SliderInt("Countdown Frames", &mCountdownTotal, MinCountdown, MaxCountdown);

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
    if (mbControlP1)
    {
        inputManager.SetP1InputMode(InputMode::Player);
        inputManager.SetP2InputMode(InputMode::Replay);
    }
    else
    {
        inputManager.SetP1InputMode(InputMode::Replay);
        inputManager.SetP2InputMode(InputMode::Player);
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
        hud.GetPause() = 0;
        hud.GetShouldStepNextFrame() = 0;
        hud.GetDisplayReplayHud() = 0;
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

    // Toggle player controlled in takeover
    if (battlePressed & (DWORD)BattleInputMask::K)
    {
        mbControlP1 = !mbControlP1;
    }

    // Replay scrubbing while paused.
    if (mMode == ReplayTakeoverMode::StandbyPaused)
    {
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

    // Restart takeover
    if (battleInput & (DWORD)BattleInputMask::PlayRecording)
    {
        mReplayManager.LoadFrame(mBookmarkFrame);
        mCountdown = 0;
        mMode = ReplayTakeoverMode::TakeoverCountdown;
        return;
    }

    // Cancel takeover
    if (battleInput & (DWORD)BattleInputMask::StartRecording)
    {
        mReplayManager.LoadFrame(mBookmarkFrame);
        ResetPlayerControl();
        mMode = ReplayTakeoverMode::StandbyPaused;
        return;
    }

    // Progress countdown
    if (mMode == ReplayTakeoverMode::TakeoverCountdown)
    {
        ++mCountdown;
        if (mCountdown >= mCountdownTotal)
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
        mReplayManager.Reset();
        if (mMode != ReplayTakeoverMode::Disabled)
        {
            mMode = ReplayTakeoverMode::Disabled;
            ResetPlayerControl();
        }
        return;
    }

    ReplayHud hud = XrdModule::GetEngine().GetReplayHud();
    if ((mMode == ReplayTakeoverMode::Disabled && !hud.GetPause()) || 
         mMode == ReplayTakeoverMode::Standby ||
         GbReplayFrameStep)
    {
        ApplySaveStateEntityUpdates();
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
            HandleTakeoverMode();
            break;
    }

    if (mMode == ReplayTakeoverMode::StandbyPaused || mMode == ReplayTakeoverMode::TakeoverCountdown)
    {
        DWORD* pause = XrdModule::GetEngine().GetPauseEngineUpdateFlag();
        if (pause != nullptr)
        {
            *pause = 1;
        }
    }
}

bool ReplayController::IsInReplayTakeoverMode() const
{
    return mMode != ReplayTakeoverMode::Disabled;
}
