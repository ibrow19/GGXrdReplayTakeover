#include <common.h>
#include <save-state.h>
#include <xrd-module.h>
#include <asw-engine.h>
#include <detours.h>
#include <cassert>
#include <entity.h>

class CreateSimpleActorDetourer
{
public:
    void DetourCreateSimpleActor(char* name, DWORD type);
    static CreateActorFunc mRealCreateSimpleActor; 
};
CreateActorFunc CreateSimpleActorDetourer::mRealCreateSimpleActor = nullptr;

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
    CreateActorFunc createActor = XrdModule::GetCreateSimpleActor();
    ForEachEntity([&createActor](DWORD entityPtr)
        {
            Entity entity(entityPtr);
            DWORD& simpleActor = entity.GetSimpleActor();
            
            // TODO: move to class managing these queries, can use 27a8 for this?
            DWORD isStateful = *(DWORD*)(entityPtr + 0x2878);

            if (!entity.IsPlayer() && simpleActor && isStateful == 0)
            {
                // If we don't null the simple actor pointer here CreateActor
                // will try and destroy the old actor. Sometimes this could work
                // fine as the actors in the loaded state won't exist, but it's
                // possible for a newly created actor to reuse the address from one 
                // of the old actors, in which case we would accidentally destroy it.
                simpleActor = 0;
                char* name = (char*)(entityPtr + 0x2858);
                DWORD type = *(DWORD*)(entityPtr + 0x287c);
                createActor((LPVOID)entityPtr, name, type);
            }
        });
}

static void DestroyNonPlayerActors()
{
    ForEachEntity([](DWORD entityPtr)
        {
            Entity entity(entityPtr);
            if (!entity.IsPlayer())
            {
                if (entity.GetSiimpeActor())
                {
                    DestroyActorFunc destroySimple = XrdModule::GetDestroySimpleActor();
                    destroySimple(entityPtr);
                }
                if (entity.GetComplexActor())
                {
                    DestroyActorFunc destroyComplex = XrdModule::GetDestroyComplexActor();
                    destroyComplex(entityPtr);
                }
            }
        });
}

static void UpdateAnimations()
{
    UpdateAnimationFunc updateAnim = XrdModule::GetUpdateComplexActorAnimation();
    ForEachEntity([&updateAnim](DWORD entityPtr)
        {
            Entity entity(entityPtr);
            if (entity.GetComplexActor())
            {
                updateAnim(entityPtr, entity.GetAnimationState(), 1);
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

    // Probably don't need this anymore as we manually destroy complex actors
    // instead, but will still call just in case.
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

    RecreateNonPlayerActors();
    CallPostLoad();
    UpdateAnimations();
}

 // This update function is only used with simple entities. We can use
 // entity fields reserved for more complicated, stateful entities to hold
 // initalisation data to recreate these entities later. 
void CreateSimpleActorDetourer::DetourCreateSimpleActor(char* name, DWORD type)
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

    mRealCreateSimpleActor((LPVOID)this, name, type);

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
    CreateSimpleActorDetourer::mRealCreateSimpleActor = XrdModule::GetCreateSimpleActor();
    void (CreateSimpleActorDetourer::* detourCreateSimpleActor)(char*, DWORD) = &CreateSimpleActorDetourer::DetourCreateSimpleActor;

    DetourTransactionBegin();
    DetourUpdateThread(GetCurrentThread());
    DetourAttach(&(PVOID&)GRealGetSaveStateTracker, DetourGetSaveStateTracker);
    DetourAttach(&(PVOID&)CreateSimpleActorDetourer::mRealCreateSimpleActor, *(PBYTE*)&detourCreateSimpleActor);
    DetourTransactionCommit();
}

void DetachSaveStateDetours()
{
    void (CreateSimpleActorDetourer::* detourCreateSimpleActor)(char*, DWORD) = &CreateSimpleActorDetourer::DetourCreateSimpleActor;

    DetourTransactionBegin();
    DetourUpdateThread(GetCurrentThread());
    DetourDetach(&(PVOID&)GRealGetSaveStateTracker, DetourGetSaveStateTracker);
    DetourDetach(&(PVOID&)CreateSimpleActorDetourer::mRealCreateSimpleActor, *(PBYTE*)&detourCreateSimpleActor);
    DetourTransactionCommit();
}

void ApplySaveStateEntityUpdates()
{
    EntityUpdateFunc update = XrdModule::GetOnlineEntityUpdate();
    ForEachEntity([&update](DWORD entity)
        {
            update(entity);
        });
}
