#include <common.h>
#include <save-state.h>
#include <detours.h>
#include <cassert>

typedef void(__fastcall* StateTransactionFunction_t)(DWORD address);

static DWORD (__fastcall * RealGetSaveStateTracker1)(DWORD manager) = NULL;
static DWORD (__stdcall * RealGetSaveStateTracker2)(void) = NULL;

static bool GbStateDetourActive = false;
static SaveStateTracker GActiveSaveStateTracker;

static const DWORD SaveStateSize = 0x23C732;
static char CustomSaveState[SaveStateSize];
static SaveStateTracker Tracker;

// TODO: naming scheme for constants like this.
// Save state is comprised 
static const int SaveStateFunctionCount = 17;
static const SaveStateSection SaveStateSections[SaveStateFunctionCount] =
{
    // 0 - main engine chunk
    { 
        0x9e20a0,
        0x9e2320,
        AswEngine,
        0x4
    },
    // 1 - Unknown
    {
        0xbad880,
        0xbad910,
        AswEngine,
        0x1c71f4
    },
    // 2 - Unknown
    {
        0xb64de0,
        0xb64e70,
        AswEngine,
        0x1c6f7c
    },
    // 3 - Unknown
    {
        0x9741d0,
        0x974270,
        Module,
        0x1738900
    },
    // 4 - Unknown
    {
        0xc0a460,
        0xc0a730,
        Module,
        0x198b7d8
    },
    // 5 - Unknown
    {
        0xc0ae70,
        0xc0aed0,
        Module,
        0x198b7e4
        
    },
    // 6 - Unknown
    {
        0xc05cf0,
        0xc060a0,
        Module,
        0x198b800 
    },
    // 7 - Unknown
    {
        0xc0b0c0,
        0xc0b320,
        Module,
        0x198b7f0 
    },
    // 8 - Unknown
    {
        0xc0b010,
        0xc0b060,
        Module,
        0x198b7f8
    },
    // 9 - Unknown
    {
        0x974f20,
        0x974fc0,
        Module,
        0x158ffe8
    },
    // 10 - Unknown
    {
        0x9c73e0,
        0x9c7490,
        AswEngine,
        0x1c7110
        
    },
    // 11 - Unknown
    {
        0x9de460,
        0x9de4f0,
        AswEngine,
        0x1c7430
        
    },
    // 12 - Unknown
    {
        0xc118e0,
        0xc119e0,
        Module,
        0x1737eb8
        
    },
    // 13 - Unknown
    {
        0xc19fe0,
        0xc1a070,
        Module,
        0x1b1da8c
        
    },
    // 14 - Unknown
    {
        0xbfdca0,
        0xbfdd30,
        Module,
        0x198b808
    },
    // 15 - Unknown
    {
        0xc0b5d0,
        0xc0ba80,
        Module,
        0x198b810
        
        
    },
    // 16 - Unknown
    {
        0xbc9140,
        0xbc91d0,
        AswEngine,
        0x1c73fc
        
    },
};

DWORD __fastcall DetourGetSaveStateTracker1(DWORD manager)
{
    if (GbStateDetourActive)
    {
        return (DWORD)&Tracker;
    }

    assert(RealGetSaveStateTracker1);
    return RealGetSaveStateTracker1(manager);
}

DWORD __fastcall DetourGetSaveStateTracker2()
{
    // TODO
//    if (GbStateDetourActive)
//    {
//        return (DWORD)((char*)&Tracker;
//    }
//
    assert(RealGetSaveStateTracker2);
    return RealGetSaveStateTracker2();
}

void DetourSaveStateTrackerFunctions()
{
    char* xrdOffset = GetModuleOffset(GameName);

    DWORD getAddress1Offset = 0xc09c60;
    DWORD getAddress2Offset = 0x4bff30;
    char* trueGetAddress1 = xrdOffset + getAddress1Offset;
    char* trueGetAddress2 = xrdOffset + getAddress2Offset;

    DetourTransactionBegin();
    DetourUpdateThread(GetCurrentThread());
    DetourAttach(&(PVOID&)RealGetSaveStateTracker1, DetourGetSaveStateTracker1);
    DetourTransactionCommit();
}

DWORD GetEngineOffset(const char* moduleOffset)
{
    DWORD* enginePtr = (DWORD*)(moduleOffset + 0x198b6e4);
    return *enginePtr;
}

void CallSaveStateFunction(const char* moduleOffset, DWORD functionOffset, SaveStateSectionBase base, DWORD stateOffset)
{
    StateTransactionFunction_t stateFunc = (StateTransactionFunction_t)(moduleOffset + functionOffset);

    DWORD stateBase = 0;
    if (base == Module)
    {
        stateBase = (DWORD)moduleOffset;
    }
    else
    {
        stateBase = GetEngineOffset(moduleOffset);
    }

    DWORD statePtr = stateBase + stateOffset;
    stateFunc(statePtr);
}

void SaveState()
{
    Tracker.saveMemCpyCount = 0;
    Tracker.saveAddress1 = (DWORD)CustomSaveState;
    Tracker.saveAddress2 = (DWORD)CustomSaveState;

    char* xrdOffset = GetModuleOffset(GameName);
    GbStateDetourActive = true;
    CallSaveStateFunction(xrdOffset, SaveStateSections[0].saveFunctionOffset, SaveStateSections[0].base, SaveStateSections[0].offset);

//    for (int i = 0; i < SaveStateFunctionCount; ++i)
//    {
//        CallSaveStateFunction(xrdOffset, SaveStateSections[i]);
//    }

    GbStateDetourActive = false;
}

void LoadState()
{
    Tracker.loadMemCpyCount = 0;
    Tracker.loadAddress = (DWORD)CustomSaveState;

    char* xrdOffset = GetModuleOffset(GameName);
    GbStateDetourActive = true;
    CallSaveStateFunction(xrdOffset, SaveStateSections[0].loadFunctionOffset, SaveStateSections[0].base, SaveStateSections[0].offset);
    GbStateDetourActive = false;
}
