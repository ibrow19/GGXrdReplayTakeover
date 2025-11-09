#include <replay-controller.h>
#include <common.h>
#include <input.h>
#include <save-state.h>
#include <xrd-module.h>
#include <asw-engine.h>
#include <replay-hud.h>
#include <entity.h>
#include <imgui.h>
#include <detours.h>

class ReplayControllerDetourer
{
public:
    void DetourSetHealth(int newHealth);
    void DetourTickSimpleActor(float delta);
    static SetHealthFunc mRealSetHealth; 
    static TickActorFunc mRealTickSimpleActor; 
};
SetHealthFunc ReplayControllerDetourer::mRealSetHealth = nullptr;
TickActorFunc ReplayControllerDetourer::mRealTickSimpleActor = nullptr;

static ReplayHudUpdateFunc GRealReplayHudUpdate = nullptr;
static UpdateTimeFunc GRealUpdateTime = nullptr;
static HandleInputsFunc GRealHandleInputs = nullptr;
static bool GbReplayFrameStep = false;
static bool GbOverrideSimpleActorPause = false;

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

ReplayController::ReplayController()
: mMode(ReplayTakeoverMode::Disabled)
, mbControlP1(true)
, mCountdownTotal(DefaultCountdown)
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

    AttachSaveStateDetours();

    GRealReplayHudUpdate = XrdModule::GetReplayHudUpdate();
    GRealUpdateTime = XrdModule::GetUpdateTime();
    GRealHandleInputs = XrdModule::GetHandleInputs();
    ReplayControllerDetourer::mRealSetHealth = XrdModule::GetSetHealth();
    ReplayControllerDetourer::mRealTickSimpleActor = XrdModule::GetTickSimpleActor();
    void (ReplayControllerDetourer::* detourSetHealth)(int) = &ReplayControllerDetourer::DetourSetHealth;
    void (ReplayControllerDetourer::* detourTickSimpleActor)(float) = &ReplayControllerDetourer::DetourTickSimpleActor;

    DetourTransactionBegin();
    DetourUpdateThread(GetCurrentThread());
    DetourAttach(&(PVOID&)GRealReplayHudUpdate, DetourReplayHudUpdate);
    DetourAttach(&(PVOID&)GRealUpdateTime, DetourUpdateTime);
    DetourAttach(&(PVOID&)GRealHandleInputs, DetourHandleInputs);
    DetourAttach(&(PVOID&)ReplayControllerDetourer::mRealSetHealth, *(PBYTE*)&detourSetHealth);
    DetourAttach(&(PVOID&)ReplayControllerDetourer::mRealTickSimpleActor, *(PBYTE*)&detourTickSimpleActor);
    DetourTransactionCommit();
}

void ReplayController::ShutdownMode()
{
    // Restore instruction to it's original value so it works in other game modes
    BYTE* instruction = XrdModule::GetControllerIndexInstruction();
    *instruction = 0x56;

    DetachSaveStateDetours();

    void (ReplayControllerDetourer::* detourSetHealth)(int) = &ReplayControllerDetourer::DetourSetHealth;
    void (ReplayControllerDetourer::* detourTickSimpleActor)(float) = &ReplayControllerDetourer::DetourTickSimpleActor;

    DetourTransactionBegin();
    DetourUpdateThread(GetCurrentThread());
    DetourDetach(&(PVOID&)GRealReplayHudUpdate, DetourReplayHudUpdate);
    DetourDetach(&(PVOID&)GRealUpdateTime, DetourUpdateTime);
    DetourDetach(&(PVOID&)GRealHandleInputs, DetourHandleInputs);
    DetourDetach(&(PVOID&)ReplayControllerDetourer::mRealSetHealth, *(PBYTE*)&detourSetHealth);
    DetourDetach(&(PVOID&)ReplayControllerDetourer::mRealTickSimpleActor, *(PBYTE*)&detourTickSimpleActor);
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
        case ReplayTakeoverMode::TakeoverRoundEnded:
            ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(200, 200, 0, 255));
            ImGui::Text("ROUND ENDED");
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

    if (ImGui::CollapsingHeader("Controls"))
    {
        if (mMode != ReplayTakeoverMode::Disabled)
        {
            ImGui::Text("Reset: Disable mod controls");
            ImGui::Text("P: Pause/unpause");
            ImGui::Text("K: Change player for takeover");
            ImGui::Text("Left/Right: rewind/advance paused replay");
            ImGui::Text("Down/Up: Fast rewind/advance paused replay");
            ImGui::Text("S/H: Single frame step back/forward");
            ImGui::Text("Play: Initiate/restart takeover");
            ImGui::Text("Record: Cancel takeover");
        }
        else
        {
            ImGui::Text("Reset: Enable mod controls");
        }
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

    // Restart takeover
    if (battleInput & (DWORD)BattleInputMask::PlayRecording)
    {
        GbOverrideSimpleActorPause = true;
        mReplayManager.LoadFrame(mBookmarkFrame);
        GbOverrideSimpleActorPause = false;
        if (mCountdownTotal == 0)
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
        GbReplayFrameStep = false;
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
