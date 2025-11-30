#include <xrd-module.h>
#include <asw-engine.h>
#include <input-manager.h>
#include <input.h>
#include <pause-menu.h>
#include <Psapi.h>
#include <cassert>

DWORD XrdModule::mBase = 0;

bool XrdModule::Init()
{
    const char* exeName = "GuiltyGearXrd.exe";
    HMODULE module = GetModuleHandleA(exeName);
    if (module == nullptr)
    {
        return false;
    }

    HANDLE process = GetCurrentProcess();
    if (process == nullptr)
    {
        return false;
    }

    MODULEINFO info;
    if (!GetModuleInformation(process, module, &info, sizeof(MODULEINFO)))
    {
        return false;
    }

    mBase = (DWORD)(info.lpBaseOfDll);
    return true;
}

bool XrdModule::IsValid()
{
    return mBase != 0;
}

DWORD XrdModule::GetBase()
{
    return mBase;
}

AswEngine XrdModule::GetEngine()
{
    return AswEngine(*(DWORD*)(mBase + 0x198b6e4));
}

InputManager XrdModule::GetInputManager()
{
    return InputManager(mBase + 0x1c77180 + 0x25380);
}

GameInputCollection XrdModule::GetGameInput()
{
    DWORD ptr1 = *(DWORD*)(mBase + 0x1c9cfa0);
    assert(ptr1);
    DWORD ptr2 = *(DWORD*)(ptr1 + 0x28);
    assert(ptr2);
    return GameInputCollection(ptr2);
}

PauseMenuButtonTable XrdModule::GetPauseMenuButtonTable()
{
    return PauseMenuButtonTable(mBase + 0x1464670);
}

DWORD& XrdModule::GetTrainingP1MaxHealth()
{
    return *(DWORD*)(mBase + 0x1ac8ab8 + 0x18);
}

DWORD& XrdModule::GetButtonDisplayMode()
{
    return *(DWORD*)(mBase + 0x1ad2918);
}

BYTE* XrdModule::GetControllerIndexInstruction()
{
    return (BYTE*)(mBase + 0x9e05f7);
}

bool XrdModule::IsPauseMenuActive()
{
    IsPauseMenuActiveFunc func = (IsPauseMenuActiveFunc)(mBase + 0xbd8cf0);
    return func();
}

DWORD& XrdModule::GetPreOrPostBattle()
{
    return *(DWORD*)(mBase + 0x1a4059c + 0x20);
}

bool XrdModule::CheckInBattle()
{
    CheckInBattleFunc func = (CheckInBattleFunc)(mBase + 0xb34eb0);
    return func();
}

DWORD XrdModule::GetUiStringTable()
{
    DWORD menuObject = *(DWORD*)(mBase + 0x1611d2c);
    assert(menuObject);
    DWORD ptr1 = *(DWORD*)(menuObject + 0x4);
    assert(ptr1);
    DWORD ptr2 = *(DWORD*)(ptr1 + 0x514);
    assert(ptr2);
    DWORD tablePtr = *(DWORD*)(ptr2 + 0xc);
    assert(tablePtr);
    return tablePtr;
}

float XrdModule::GetReplayTextSpacing()
{
    return *(float*)(mBase + 0x10a1148);
}

float& XrdModule::GetDefaultTickDelta()
{
    return *(float*)(mBase + 0x109f00c);
}

EntityUpdateFunc XrdModule::GetOnlineEntityUpdate()
{
    return (EntityUpdateFunc)(mBase + 0xb6efd0);
}

MainGameLogicFunc XrdModule::GetMainGameLogic()
{
    return (MainGameLogicFunc)(mBase + 0xa61240);
}

MainGameLogicFunc XrdModule::GetOfflineMainGameLogic()
{
    return (MainGameLogicFunc)(mBase + 0xa5df00);
}

SetStringFunc XrdModule::GetSetString()
{
    return (SetStringFunc)(mBase + 0x973390);
}

ReplayHudUpdateFunc XrdModule::GetReplayHudUpdate()
{
    return (ReplayHudUpdateFunc)(mBase + 0xbc30a0);
}

DisplayReplayHudMenuFunc XrdModule::GetDisplayReplayHudMenu()
{
    return (DisplayReplayHudMenuFunc)(mBase + 0xbc5430);
}

AddUiTextFunc XrdModule::GetAddUiText()
{
    return (AddUiTextFunc)(mBase + 0xba7ac0);
}

GetSaveStateTrackerFunc XrdModule::GetSaveStateTrackerGetter()
{
    return (GetSaveStateTrackerFunc)(mBase + 0xc09c60);
}

DWORD& XrdModule::GetSaveStateTrackerPtr()
{
    return *(DWORD*)(mBase + 0x16da4f0);
}

EntityActorManagementFunc XrdModule::GetPreStateLoad()
{
    return (EntityActorManagementFunc)(mBase + 0x9ec230);
}

EntityActorManagementFunc XrdModule::GetPostStateLoad()
{
    return (EntityActorManagementFunc)(mBase + 0x9e2030);
}

CreateActorFunc XrdModule::GetCreateSimpleActor()
{
    return (CreateActorFunc)(mBase + 0xa0e020);
}

DestroyActorFunc XrdModule::GetDestroySimpleActor()
{
    return (DestroyActorFunc)(mBase + 0xa08c20);
}

DestroyActorFunc XrdModule::GetDestroyComplexActor()
{
    return (DestroyActorFunc)(mBase + 0xb6a7b0);
}

UpdateAnimationFunc XrdModule::GetUpdateComplexActorAnimation()
{
    return (UpdateAnimationFunc)(mBase + 0xa11ea0);
}

SetGameModeFunc XrdModule::GetGameModeSetter()
{
    return (SetGameModeFunc)(mBase + 0xb2ea20);
}

SetHealthFunc XrdModule::GetSetHealth()
{
    return (SetHealthFunc)(mBase + 0xa05060);
}

UpdateTimeFunc XrdModule::GetUpdateTime()
{
    return (UpdateTimeFunc)(mBase + 0xbaab80);
}

HandleInputsFunc XrdModule::GetHandleInputs()
{
    return (HandleInputsFunc)(mBase + 0x9e0290);
}

TickActorFunc XrdModule::GetTickActor()
{
    return (TickActorFunc)(mBase + 0x275720);
}

TickActorFunc XrdModule::GetTickSimpleActor()
{
    return (TickActorFunc)(mBase + 0x6ee9c0);
}

TickRelevantActorsFunc XrdModule::GetTickRelevantActors()
{
    return (TickRelevantActorsFunc)(mBase + 0xa5d670);
}

IsResimulatingFunc XrdModule::GetIsResimulating()
{
    return (IsResimulatingFunc)(mBase + 0xbfbd20);
}
