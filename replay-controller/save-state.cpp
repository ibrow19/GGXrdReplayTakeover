#include <common.h>
#include <save-state.h>
#include <xrd-module.h>
#include <asw-engine.h>
#include <detours.h>
#include <cassert>
#include <entity.h>

class CreateEntityActorDetourer
{
public:
    void DetourCreateEntityActor(char* name, DWORD type);
    static CreateActorFunc mRealCreateEntityActor; 
};
CreateActorFunc CreateEntityActorDetourer::mRealCreateEntityActor = nullptr;

static GetSaveStateTrackerFunc GRealGetSaveStateTracker = nullptr;
static bool GbStateDetourActive = false;
static SaveStateTracker GTracker;

// Save state is comprised 
static constexpr int SaveStateFunctionCount = 17;
static const SaveStateSection SaveStateSections[SaveStateFunctionCount] =
{
    // 0 - main engine chunk
    { 
        0x9e20a0,
        0x9e2320,
        SaveStateSectionBase::Engine,
        0x4
    },
    // 1 - Unknown
    {
        0xbad880,
        0xbad910,
        SaveStateSectionBase::Engine,
        0x1c71f4
    },
    // 2 - Unknown
    {
        0xb64de0,
        0xb64e70,
        SaveStateSectionBase::Engine,
        0x1c6f7c
    },
    // 3 - Unknown
    {
        0x9741d0,
        0x974270,
        SaveStateSectionBase::Module,
        0x1738900
    },
    // 4 - Unknown
    {
        0xc0a460,
        0xc0a730,
        SaveStateSectionBase::Module,
        0x198b7d8
    },
    // 5 - Unknown
    {
        0xc0ae70,
        0xc0aed0,
        SaveStateSectionBase::Module,
        0x198b7e4
        
    },
    // 6 - Unknown
    {
        0xc05cf0,
        0xc060a0,
        SaveStateSectionBase::Module,
        0x198b800 
    },
    // 7 - Unknown
    {
        0xc0b0c0,
        0xc0b320,
        SaveStateSectionBase::Module,
        0x198b7f0 
    },
    // 8 - Unknown
    {
        0xc0b010,
        0xc0b060,
        SaveStateSectionBase::Module,
        0x198b7f8
    },
    // 9 - Unknown
    {
        0x974f20,
        0x974fc0,
        SaveStateSectionBase::Module,
        0x158ffe8
    },
    // 10 - Unknown
    {
        0x9c73e0,
        0x9c7490,
        SaveStateSectionBase::Engine,
        0x1c7110
        
    },
    // 11 - Unknown
    {
        0x9de460,
        0x9de4f0,
        SaveStateSectionBase::Engine,
        0x1c7430
        
    },
    // 12 - Unknown
    {
        0xc118e0,
        0xc119e0,
        SaveStateSectionBase::Module,
        0x1737eb8
        
    },
    // 13 - Unknown
    {
        0xc19fe0,
        0xc1a070,
        SaveStateSectionBase::Module,
        0x1b1da8c
        
    },
    // 14 - Unknown
    {
        0xbfdca0,
        0xbfdd30,
        SaveStateSectionBase::Module,
        0x198b808
    },
    // 15 - Unknown
    {
        0xc0b5d0,
        0xc0ba80,
        SaveStateSectionBase::Module,
        0x198b810
        
        
    },
    // 16 - Unknown
    {
        0xbc9140,
        0xbc91d0,
        SaveStateSectionBase::Engine,
        0x1c73fc
        
    },
};

static void CallSaveStateFunction(DWORD functionOffset, SaveStateSectionBase base, DWORD stateOffset)
{
    const DWORD moduleBase = XrdModule::GetBase();

    typedef void(__fastcall* StateTransactionFunc)(DWORD address);
    StateTransactionFunc stateFunc = (StateTransactionFunc)(moduleBase + functionOffset);

    DWORD stateBase = 0;
    if (base == SaveStateSectionBase::Module)
    {
        stateBase = (DWORD)moduleBase;
    }
    else
    {
        stateBase = XrdModule::GetEngine().GetPtr();
    }

    DWORD statePtr = stateBase + stateOffset;
    stateFunc(statePtr);
}

static void CallPreLoad()
{
    DWORD engineOffset = XrdModule::GetEngine().GetOffset4();
    EntityActorManagementFunc func = XrdModule::GetPreStateLoad();
    func(engineOffset);
}

static void CallPostLoad()
{
    DWORD engineOffset = XrdModule::GetEngine().GetOffset4();
    EntityActorManagementFunc func = XrdModule::GetPostStateLoad();
    func(engineOffset);
}

static void RecreateNonPlayerActors()
{
    CreateActorFunc createActor = XrdModule::GetCreateEntityActor();
    ForEachEntity([&createActor](DWORD entity)
        {
            // TODO: move to class managing these queries.
            DWORD actorPtr = *(DWORD*)(entity + 0x27cc);
            DWORD isPlayer = *(DWORD*)(entity + 0x10);
            DWORD isStateful = *(DWORD*)(entity + 0x2878);

            if (isPlayer == 0 && actorPtr != NULL && isStateful == 0)
            {
                char* name = (char*)(entity + 0x2858);
                DWORD type = *(DWORD*)(entity + 0x287c);
                createActor((LPVOID)entity, name, type);
            }
        });
}

static void DestroyNonPlayerActors()
{
    DestroyActorFunc destroyActor = XrdModule::GetDestroyEntityActor();
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

static void UpdatePlayerAnimations()
{
    UpdatePlayerAnimFunc updateAnim = XrdModule::GetUpdatePlayerAnim();
    ForEachEntity([&updateAnim](DWORD entity)
        {
            if (Entity(entity).IsPlayer())
            {
                char* currentStateName = (char*)(entity + 0xa58);
                updateAnim(entity, currentStateName, 1);
            }
        });
}

void SaveState(char* dest)
{
    GTracker.saveMemCpyCount = 0;
    GTracker.saveAddress1 = (DWORD)dest;
    GTracker.saveAddress2 = (DWORD)dest;

    DWORD& trackerPtr = XrdModule::GetSaveStateTrackerPtr();
    trackerPtr = (DWORD)&GTracker;

    GbStateDetourActive = true;

    for (int i = 0; i < SaveStateFunctionCount; ++i)
    {
        CallSaveStateFunction(
            SaveStateSections[i].saveFunctionOffset, 
            SaveStateSections[i].base, 
            SaveStateSections[i].offset);
    }

    // If we don't reset the save slot ptr then the game will immediately crash
    // trying to check it when exiting the current game.
    trackerPtr = NULL;
    GbStateDetourActive = false;
}

void LoadState(const char* src)
{
    DestroyNonPlayerActors();
    CallPreLoad();

    GTracker.loadMemCpyCount = 0;
    GTracker.loadAddress = (DWORD)src;

    DWORD& trackerPtr = XrdModule::GetSaveStateTrackerPtr();
    trackerPtr = (DWORD)&GTracker;

    GbStateDetourActive = true;

    for (int i = 0; i < SaveStateFunctionCount; ++i)
    {
        CallSaveStateFunction(
            SaveStateSections[i].loadFunctionOffset, 
            SaveStateSections[i].base, 
            SaveStateSections[i].offset);
    }
    GbStateDetourActive = false;
    trackerPtr = NULL;

    CallPostLoad();
    RecreateNonPlayerActors();
    UpdatePlayerAnimations();
}

 // This update function is only used with simple entities. We can use
 // entity fields reserved for more complicated, stateful entities to hold
 // initalisation data to recreate these entities later. 
void CreateEntityActorDetourer::DetourCreateEntityActor(char* name, DWORD type)
{
    bool bRecreatingSimple = *(DWORD*)((DWORD)this + 0x27cc) != 0 && *(DWORD*)((DWORD)this + 0x2878) == 0;

    // TODO: It should be impossible for type to be 0 here unless we're doing
    // something wrong storing type in the entity. However, it still sometimes
    // happens and causes a crash, seems common with Answer. As a temporary
    // workaround forcing type to 0x17 seems to work as it is usually (always?)
    // this value.
    if (type == 0)
    {
        type = 0x17;
    }

    mRealCreateEntityActor((LPVOID)this, name, type);

    // Don't overwrite these values if the entity needs them.
    // Ignore entities where the simple actor (27cc) is already set as that
    // means we're resuing an existing simple actor and need to reset our custom changes.
    if (!bRecreatingSimple &&
            (*(DWORD*)((DWORD)this + 0x2878) != 0 ||
             *(DWORD*)((DWORD)this + 0x2858) != 0 ||
             *(DWORD*)((DWORD)this + 0x287c) != 0))
    {
        return;
    }

    SetStringFunc setString = XrdModule::GetSetString();
    char* stateName = (char*)((DWORD)this + 0x2858);
    setString(stateName, name);

    *(DWORD*)((DWORD)this + 0x287c) = type;
}

static DWORD __fastcall DetourGetSaveStateTracker(DWORD manager)
{
    if (GbStateDetourActive)
    {
        return (DWORD)&GTracker.getTrackerAnchor;
    }

    assert(GRealGetSaveStateTracker);
    return GRealGetSaveStateTracker(manager);
}

void AttachSaveStateDetours()
{
    GRealGetSaveStateTracker = XrdModule::GetSaveStateTrackerGetter();
    CreateEntityActorDetourer::mRealCreateEntityActor = XrdModule::GetCreateEntityActor();
    void (CreateEntityActorDetourer::* detourCreateEntityActor)(char*, DWORD) = &CreateEntityActorDetourer::DetourCreateEntityActor;

    DetourTransactionBegin();
    DetourUpdateThread(GetCurrentThread());
    DetourAttach(&(PVOID&)GRealGetSaveStateTracker, DetourGetSaveStateTracker);
    DetourAttach(&(PVOID&)CreateEntityActorDetourer::mRealCreateEntityActor, *(PBYTE*)&detourCreateEntityActor);
    DetourTransactionCommit();
}

void DetachSaveStateDetours()
{
    GRealGetSaveStateTracker = nullptr;
    CreateEntityActorDetourer::mRealCreateEntityActor = nullptr;
    void (CreateEntityActorDetourer::* detourCreateEntityActor)(char*, DWORD) = &CreateEntityActorDetourer::DetourCreateEntityActor;

    DetourTransactionBegin();
    DetourUpdateThread(GetCurrentThread());
    DetourDetach(&(PVOID&)GRealGetSaveStateTracker, DetourGetSaveStateTracker);
    DetourDetach(&(PVOID&)CreateEntityActorDetourer::mRealCreateEntityActor, *(PBYTE*)&detourCreateEntityActor);
    DetourTransactionCommit();
}
