#include <replay-mods.h>
#include <replay-controller.h>
#include <replay-hud.h>
#include <xrd-module.h>
#include <asw-engine.h>
#include <entity.h>
#include <save-state.h>
#include <common.h>
#include <detours.h>
#include <audio.h>
#include <cassert>

class ReplayDetourer
{
public:
    void DetourSetHealth(int newHealth);
    void DetourTickSimpleActor(float delta);
    void DetourDisplayReplayHudMenu();
    static SetHealthFunc mRealSetHealth; 
    static TickActorFunc mRealTickSimpleActor; 
    static DisplayReplayHudMenuFunc mRealDisplayReplayHudMenu; 
};
SetHealthFunc ReplayDetourer::mRealSetHealth = nullptr;
TickActorFunc ReplayDetourer::mRealTickSimpleActor = nullptr;
DisplayReplayHudMenuFunc ReplayDetourer::mRealDisplayReplayHudMenu = nullptr;

static ReplayHudUpdateFunc GRealReplayHudUpdate = nullptr;
static AddUiTextFunc GRealAddUiText = nullptr;
static UpdateTimeFunc GRealUpdateTime = nullptr;
static HandleInputsFunc GRealHandleInputs = nullptr;

namespace SoundSettings
{
    static float cachedEffectVolume = 0.f;
    static float cachedVoiceVolume = 0.f;
    static float cachedSuperVoiceVolume = 0.f;
    static float cachedSystemVoiceVolume = 0.f;
    static bool bCached = false;
}

void ReplayDetourer::DetourSetHealth(int newHealth)
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
void ReplayDetourer::DetourTickSimpleActor(float delta)
{
    ReplayController* controller = GameModeController::Get<ReplayController>();
    assert(controller);
    if (ReplayDetourSettings::bOverrideSimpleActorPause || !controller->IsPaused())
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

void ReplayDetourer::DetourDisplayReplayHudMenu()
{
    ReplayDetourSettings::bOverrideHudText = true;
    ReplayDetourSettings::bAddingFirstTextRow = true;

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
        DWORD& nextRound = hud.GetGoToNextRound();
        DWORD& cameraUnavailable = hud.GetCameraUnavailable();
        DWORD realShouldPause = shouldPause;
        DWORD realNextRound = nextRound;
        DWORD realCameraUnavailable = cameraUnavailable;
        shouldPause = 1;
        nextRound = 0;
        cameraUnavailable = 0;

        mRealDisplayReplayHudMenu((LPVOID)this);

        shouldPause = realShouldPause;
        nextRound = realNextRound;
        cameraUnavailable = realCameraUnavailable;
    }
    ReplayDetourSettings::bOverrideHudText = false;
}

void DetourAddUiText(DWORD* textParams, DWORD param1, DWORD param2, DWORD param3, DWORD param4, DWORD param5)
{
    GRealAddUiText(textParams, param1, param2, param3, param4, param5);
    if (ReplayDetourSettings::bOverrideHudText)
    {
        if (ReplayDetourSettings::bAddingFirstTextRow)
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
            ReplayDetourSettings::bAddingFirstTextRow = false;
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
            ReplayDetourSettings::bReplayFrameStep = true;
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

void AddReplayMods()
{
    // Reset Settings
    ReplayDetourSettings::bReplayFrameStep = false;
    ReplayDetourSettings::bOverrideSimpleActorPause = false;
    ReplayDetourSettings::bOverrideHudText = false;
    ReplayDetourSettings::bAddingFirstTextRow = false;
    SoundSettings::bCached = false;

    // Make regions writable for instruction editing.

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
    BYTE* controllerInstruction = XrdModule::GetControllerIndexInstruction();
    MakeRegionWritable((DWORD)controllerInstruction, 1);
    *controllerInstruction = 0x52;

    // Make instructions for controlling input display
    // writable so we can change them when we need to later.
    MakeRegionWritable((DWORD)XrdModule::GetInputDisplayInstruction(), 1);

    // Attach detours
    GRealReplayHudUpdate = XrdModule::GetReplayHudUpdate();
    GRealAddUiText = XrdModule::GetAddUiText();
    GRealUpdateTime = XrdModule::GetUpdateTime();
    GRealHandleInputs = XrdModule::GetHandleInputs();
    ReplayDetourer::mRealSetHealth = XrdModule::GetSetHealth();
    ReplayDetourer::mRealTickSimpleActor = XrdModule::GetTickSimpleActor();
    ReplayDetourer::mRealDisplayReplayHudMenu = XrdModule::GetDisplayReplayHudMenu();
    void (ReplayDetourer::* detourSetHealth)(int) = &ReplayDetourer::DetourSetHealth;
    void (ReplayDetourer::* detourTickSimpleActor)(float) = &ReplayDetourer::DetourTickSimpleActor;
    void (ReplayDetourer::* detourDisplayReplayHudMenu)(void) = &ReplayDetourer::DetourDisplayReplayHudMenu;

    DetourTransactionBegin();
    DetourUpdateThread(GetCurrentThread());
    DetourAttach(&(PVOID&)GRealReplayHudUpdate, DetourReplayHudUpdate);
    DetourAttach(&(PVOID&)GRealAddUiText, DetourAddUiText);
    DetourAttach(&(PVOID&)GRealUpdateTime, DetourUpdateTime);
    DetourAttach(&(PVOID&)GRealHandleInputs, DetourHandleInputs);
    DetourAttach(&(PVOID&)ReplayDetourer::mRealSetHealth, *(PBYTE*)&detourSetHealth);
    DetourAttach(&(PVOID&)ReplayDetourer::mRealTickSimpleActor, *(PBYTE*)&detourTickSimpleActor);
    DetourAttach(&(PVOID&)ReplayDetourer::mRealDisplayReplayHudMenu, *(PBYTE*)&detourDisplayReplayHudMenu);
    DetourTransactionCommit();

    AttachSaveStateDetours();
}

void RemoveReplayMods()
{
    // Restore instruction to their original values.
    BYTE* instruction = XrdModule::GetControllerIndexInstruction();
    *instruction = 0x56;

    EnableSoundEffects();
    EnableInputDisplay();

    // Detach detours
    void (ReplayDetourer::* detourSetHealth)(int) = &ReplayDetourer::DetourSetHealth;
    void (ReplayDetourer::* detourTickSimpleActor)(float) = &ReplayDetourer::DetourTickSimpleActor;
    void (ReplayDetourer::* detourDisplayReplayHudMenu)(void) = &ReplayDetourer::DetourDisplayReplayHudMenu;

    DetourTransactionBegin();
    DetourUpdateThread(GetCurrentThread());
    DetourDetach(&(PVOID&)GRealReplayHudUpdate, DetourReplayHudUpdate);
    DetourDetach(&(PVOID&)GRealAddUiText, DetourAddUiText);
    DetourDetach(&(PVOID&)GRealUpdateTime, DetourUpdateTime);
    DetourDetach(&(PVOID&)GRealHandleInputs, DetourHandleInputs);
    DetourDetach(&(PVOID&)ReplayDetourer::mRealSetHealth, *(PBYTE*)&detourSetHealth);
    DetourDetach(&(PVOID&)ReplayDetourer::mRealTickSimpleActor, *(PBYTE*)&detourTickSimpleActor);
    DetourDetach(&(PVOID&)ReplayDetourer::mRealDisplayReplayHudMenu, *(PBYTE*)&detourDisplayReplayHudMenu);
    DetourTransactionCommit();

    DetachSaveStateDetours();
}

void DisableSoundEffects()
{
    SoundData soundData = XrdModule::GetSoundData();
    SoundSettings::cachedEffectVolume = soundData.GetBattleEffectVolume();
    SoundSettings::cachedVoiceVolume = soundData.GetBattleVoiceVolume();
    SoundSettings::cachedSuperVoiceVolume = soundData.GetBattleSuperVoiceVolume();
    SoundSettings::cachedSystemVoiceVolume = soundData.GetBattleSystemVoiceVolume();

    soundData.GetBattleEffectVolume() = 0.f;
    soundData.GetBattleVoiceVolume() = 0.f;
    soundData.GetBattleSuperVoiceVolume() = 0.f;
    soundData.GetBattleSystemVoiceVolume() = 0.f;

    SoundSettings::bCached = true;
}

void EnableSoundEffects()
{
    if (SoundSettings::bCached)
    {
        SoundSettings::bCached = false;
        SoundData soundData = XrdModule::GetSoundData();
        soundData.GetBattleEffectVolume() = SoundSettings::cachedEffectVolume;
        soundData.GetBattleVoiceVolume() = SoundSettings::cachedVoiceVolume;
        soundData.GetBattleSuperVoiceVolume() = SoundSettings::cachedSuperVoiceVolume;
        soundData.GetBattleSuperVoiceVolume() = SoundSettings::cachedSuperVoiceVolume;
    }
}

// The input display is added to the screen every frame. If we do this while
// simulating multiple frames then the game will render all of them at once
// after we finish simulating. So we edit an instruction to jump over the
// code displaying the input. The condition we're changing is checking the
// result of a function checking if we're online + resimulating for a rollback;
// so we could detour that function as an alternate solution. But I'm not
// confident about how that would effect other parts of the code that call the
// same function.
void DisableInputDisplay()
{
    // Change to a jmp
    BYTE* instruction = XrdModule::GetInputDisplayInstruction();
    *instruction = 0xeb;
}

void EnableInputDisplay()
{
    // Restore original jne instruction
    BYTE* instruction = XrdModule::GetInputDisplayInstruction();
    *instruction = 0x75;
}
