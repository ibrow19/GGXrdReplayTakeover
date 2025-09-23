#include <xrd-module.h>
#include <asw-engine.h>
#include <input-manager.h>
#include <input.h>
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
    assert(ptr1 != 0);
    DWORD ptr2 = *(DWORD*)(ptr1 + 0x28);
    assert(ptr2 != 0);
    return GameInputCollection(ptr2);
}

BYTE* XrdModule::GetControllerIndexInstruction()
{
    return (BYTE*)(mBase + 0x9e05f7);
}

EntityUpdateFunc XrdModule::GetOnlineEntityUpdate()
{
    return (EntityUpdateFunc)(mBase + 0xb6efd0);
}

MainGameLogicFunc XrdModule::GetMainGameLogic()
{
    return (MainGameLogicFunc)(mBase + 0xa61240);
}

SetStringFunc XrdModule::GetSetString()
{
    return (SetStringFunc)(mBase + 0x973390);
}

ReplayHudUpdateFunc XrdModule::GetReplayHudUpdate()
{
    return (ReplayHudUpdateFunc)(mBase + 0xbc30a0);
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

CreateActorFunc XrdModule::GetCreateEntityActor()
{
    return (CreateActorFunc)(mBase + 0xa0e020);
}

DestroyActorFunc XrdModule::GetDestroyEntityActor()
{
    return (DestroyActorFunc)(mBase + 0xa08c20);
}

UpdatePlayerAnimFunc XrdModule::GetUpdatePlayerAnim()
{
    return (UpdatePlayerAnimFunc)(mBase + 0xa11ea0);
}

SetGameModeFunc XrdModule::GetGameModeSetter()
{
    return (SetGameModeFunc)(mBase + 0xb2ea20);
}
