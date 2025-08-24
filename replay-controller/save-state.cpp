#include <common.h>
#include <save-state.h>
#include <detours.h>
#include <cassert>

typedef void(__fastcall* StateTransactionFunction_t)(DWORD address);
typedef DWORD(__fastcall* RealGetSaveStateTracker1_t)(DWORD manager);

typedef BYTE(__fastcall* GetRenderStateUpToDate_t)(void);

typedef void(__fastcall* EntityActorManagementFunction_t)(DWORD aswEngine);
//typedef BYTE(__stdcall* ExtraPostLoadFunction1_t)(void);
typedef void(__stdcall* ExtraPostLoadFunction2_t)(void);

static RealGetSaveStateTracker1_t GRealGetSaveStateTracker1 = NULL;
static DWORD (__stdcall* GRealGetSaveStateTracker2)(void) = NULL;

static bool GbStateDetourActive = false;
static SaveStateTracker GActiveSaveStateTracker;

static const DWORD SaveStateSize = 0x23C732;
static char CustomSaveState[SaveStateSize];
static SaveStateTracker Tracker;
static SaveStateTrackerIndirection TrackerIndirection;
static SaveStateTrackerPointer TrackerPtr;


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
        return (DWORD)&Tracker.getTrackerAnchor;
    }

    assert(GRealGetSaveStateTracker1);
    return GRealGetSaveStateTracker1(manager);
}

DWORD __fastcall DetourGetSaveStateTracker2()
{
    // TODO
//    if (GbStateDetourActive)
//    {
//        return (DWORD)((char*)&Tracker;
//    }
//
    assert(GRealGetSaveStateTracker2);
    return GRealGetSaveStateTracker2();
}

BYTE __fastcall DetourGetRenderStateUpToDate()
{
    return 0;
}

//void DetourRenderStateFunction()
//{
//    char* xrdOffset = GetModuleOffset(GameName);
//
//    DWORD funcOffset = 0xbfb840;
//    char* trueGetTracker1 = xrdOffset + funcOffset;
//
//    RealGetSaveStateTracker1 = (RealGetSaveStateTracker1_t)trueGetTracker1;
//
//    DetourTransactionBegin();
//    DetourUpdateThread(GetCurrentThread());
//    DetourAttach(&(PVOID&)RealGetSaveStateTracker1, DetourGetSaveStateTracker1);
//    DetourTransactionCommit();
//
//}

void DetourSaveStateTrackerFunctions()
{
    char* xrdOffset = GetModuleOffset(GameName);

    DWORD getTracker1Offset = 0xc09c60;
    DWORD getTracker2Offset = 0x4bff30;
    char* trueGetTracker1 = xrdOffset + getTracker1Offset;
    char* trueGetTracker2 = xrdOffset + getTracker2Offset;

    GRealGetSaveStateTracker1 = (RealGetSaveStateTracker1_t)trueGetTracker1;

    DetourTransactionBegin();
    DetourUpdateThread(GetCurrentThread());
    DetourAttach(&(PVOID&)GRealGetSaveStateTracker1, DetourGetSaveStateTracker1);
    DetourTransactionCommit();
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

void CallPreLoad()
{
    char* xrdOffset = GetModuleOffset(GameName);
    DWORD engineOffset = GetEngineOffset(xrdOffset) + 4;
    EntityActorManagementFunction_t actorsFunc = (EntityActorManagementFunction_t)(xrdOffset + 0x9ec230);
    actorsFunc(engineOffset);
}

void CallPostLoad()
{
    char* xrdOffset = GetModuleOffset(GameName);
    DWORD engineOffset = GetEngineOffset(xrdOffset) + 4;
    EntityActorManagementFunction_t actorsFunc = (EntityActorManagementFunction_t)(xrdOffset + 0x9e2030);
    actorsFunc(engineOffset);
}

typedef BYTE(__thiscall* ExtraPostLoadFunction1_t)(DWORD rollbackManager);
void CallExtraPostLoad1()
{
    char* xrdOffset = GetModuleOffset(GameName);
    ExtraPostLoadFunction1_t func = (ExtraPostLoadFunction1_t)(xrdOffset + 0xbfcc60);
    func(0);
}

void CallExtraPostLoad2()
{
    char* xrdOffset = GetModuleOffset(GameName);
    ExtraPostLoadFunction2_t func = (ExtraPostLoadFunction2_t)(xrdOffset + 0xbfbbb0);
    func();
}

void SaveState()
{
    Tracker.saveMemCpyCount = 0;
    Tracker.saveAddress1 = (DWORD)CustomSaveState;
    Tracker.saveAddress2 = (DWORD)CustomSaveState;

    char* xrdOffset = GetModuleOffset(GameName);

    // point to rollback manager to tracker
    DWORD* saveSlotPtr = (DWORD*)(xrdOffset + 0x16da4f0);
    *saveSlotPtr = (DWORD)&Tracker;

    GbStateDetourActive = true;

    for (int i = 0; i < SaveStateFunctionCount; ++i)
    {
        CallSaveStateFunction(
            xrdOffset, 
            SaveStateSections[i].saveFunctionOffset, 
            SaveStateSections[i].base, 
            SaveStateSections[i].offset);
    }

    // If we don't reset the save slot ptr then the game will immediately crash
    // trying to check it when exiting the current game.
    *saveSlotPtr = NULL;

    GbStateDetourActive = false;
}

typedef void(__thiscall* RecreateActor_t)(DWORD thisArg, DWORD stateName, DWORD unknown);
void RecreateNonPlayerActors()
{
    char* xrdOffset = GetModuleOffset(GameName);
    DWORD recreateActorOffset = 0xa0e020;
    RecreateActor_t recreateActor = (RecreateActor_t)(xrdOffset + recreateActorOffset);
    
    ForEachEntity([&recreateActor](DWORD entity)
        {
            // TODO: move to class managing these queries.
            DWORD actorPtr = *(DWORD*)(entity + 0x27cc);
            DWORD isPlayer = *(DWORD*)(entity + 0x10);
            DWORD isStateful = *(DWORD*)(entity + 0x2878);

            if (isPlayer == 0 && actorPtr != NULL && isStateful == 0)
            {
                DWORD name = entity + 0x2858;
                DWORD type = *(DWORD*)(entity + 0x287c);
                recreateActor(entity, name, type);
            }
        });
}

typedef void(__fastcall* DestroyActor_t)(DWORD Entity);
void DestroyNonPlayerActors()
{
    char* xrdOffset = GetModuleOffset(GameName);
    DWORD destroyActorOffset = 0xa08c20;
    DestroyActor_t destroyActor = (DestroyActor_t)(xrdOffset + destroyActorOffset);
    
    ForEachEntity([&destroyActor](DWORD entity)
        {
            // TODO: refactor to avoid rewriting 27cc everythere etc.
            DWORD actorPtr = *(DWORD*)(entity + 0x27cc);
            DWORD isPlayer = *(DWORD*)(entity + 0x10);
            if (isPlayer == 0 && actorPtr != NULL)
            {
                destroyActor(entity);
            }
        });
}

void LoadState()
{
    DestroyNonPlayerActors();
    CallPreLoad();

    Tracker.loadMemCpyCount = 0;
    Tracker.loadAddress = (DWORD)CustomSaveState;

    char* xrdOffset = GetModuleOffset(GameName);

    // TODO: move to function.
    // point to rollback manager to tracker
    DWORD* saveSlotPtr = (DWORD*)(xrdOffset + 0x16da4f0);
    *saveSlotPtr = (DWORD)&Tracker;

    GbStateDetourActive = true;

    for (int i = 0; i < SaveStateFunctionCount; ++i)
    {
        CallSaveStateFunction(
            xrdOffset, 
            SaveStateSections[i].loadFunctionOffset, 
            SaveStateSections[i].base, 
            SaveStateSections[i].offset);
    }
    GbStateDetourActive = false;
    *saveSlotPtr = NULL;

    CallPostLoad();
    RecreateNonPlayerActors();
}
