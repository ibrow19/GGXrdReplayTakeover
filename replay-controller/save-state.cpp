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

static void RecreateSimpleActors()
{
    ForEachEntity([](DWORD entityPtr, DWORD index, void* extraData)
        {
            Entity entity(entityPtr);
            DWORD& simpleActor = entity.GetSimpleActor();
            
            if (!entity.IsPlayer() && simpleActor)
            {
                // If we don't null the simple actor pointer here CreateActor
                // will try and destroy the old actor. Sometimes this could work
                // fine as the actors in the loaded state won't exist, but it's
                // possible for a newly created actor to reuse the address from one 
                // of the old actors, in which case we would accidentally destroy it.
                simpleActor = 0;

                // Currently working under the assumption that this parameter
                // should always be 0x17 based on code at xrd + 0xa126a4. Other
                // parts of the code call it with 0x16 but I don't know why/if
                // they are used in regular gameplay.
                DWORD type = 0x17;
                char* name = (char*)entity.GetSimpleActorSaveData();
                CreateActorFunc createActor = XrdModule::GetCreateSimpleActor();
                createActor((LPVOID)entityPtr, name, type);
            }
        });
}

static void ResetGameState()
{
    AswEngine engine = XrdModule::GetEngine();
    ResetGameFunc resetGame = XrdModule::GetResetGame();
    DestroyAllSimpleActorsFunc destroySimpleActors = XrdModule::GetDestroyAllSimpleActors();
    resetGame(engine.GetOffset4(), 1);
    destroySimpleActors(engine.GetGameLogicManager().GetPtr());
}

static void UpdateAnimations(const EntitySaveData* entitySaveData)
{
    ForEachEntity([](DWORD entityPtr, DWORD index, void* extraData)
        {
            Entity entity(entityPtr);
            if (entity.GetComplexActor())
            {
                UpdateAnimationFunc updateAnim = XrdModule::GetUpdateComplexActorAnimation();
                updateAnim(entityPtr, entity.GetAnimationFrameName(), 1);
            }

            DWORD simpleActor = entity.GetSimpleActor();
            if (simpleActor)
            {
                EntitySaveData* saveData = (EntitySaveData*)extraData;
                TickActorFunc tickActor = XrdModule::GetTickSimpleActor();
                float delta = saveData[index].simpleActorTime;
                TimeStepData timeStepData = SimpleActor(simpleActor).GetTimeStepData();
                if (timeStepData.IsValid() && timeStepData.ShouldUseFixedTimeStep())
                {
                    float& fixedTimeStep = timeStepData.GetFixedTimeStep();
                    float oldTimeStep = fixedTimeStep;
                    fixedTimeStep = delta;
                    tickActor((LPVOID)simpleActor, 0);
                    fixedTimeStep = oldTimeStep;
                }
                else
                {
                    tickActor((LPVOID)simpleActor, delta);
                }
            }
        },
        (void*)entitySaveData);
}

// Apply online entity update that adds extra data needed for save state
// loading. Also store custom data needed for restoring simple actors.
static void PreSaveEntityUpdates(EntitySaveData* entitySaveData)
{
    ForEachEntity([](DWORD entityPtr, DWORD index, void* extraData)
        {
            Entity entity(entityPtr);
            EntitySaveData* saveData = (EntitySaveData*)extraData;
            DWORD simpleActor = entity.GetSimpleActor();
            DWORD namePtr = 0;

            if (simpleActor)
            {
                namePtr = entity.GetSimpleActorSaveData();
                saveData[index].simpleActorTime = SimpleActor(simpleActor).GetTime();
            }

            EntityUpdateFunc update = XrdModule::GetOnlineEntityUpdate();
            update(entityPtr);

            // Restore name in entity as it will have been overwritten by the online entity update.
            if (simpleActor)
            {
                entity.GetSimpleActorSaveData() = namePtr;
            }
        },
        entitySaveData);
}

void SaveState(SaveData& dest)
{
    PreSaveEntityUpdates(dest.entityData);

    GTracker.saveMemCpyCount = 0;
    GTracker.saveAddress1 = (DWORD)dest.xrdData;
    GTracker.saveAddress2 = (DWORD)dest.xrdData;

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

void LoadState(const SaveData& src)
{
    ResetGameState();

    GTracker.loadMemCpyCount = 0;
    GTracker.loadAddress = (DWORD)src.xrdData;

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

    RecreateSimpleActors();
    CallPostLoad();
    UpdateAnimations(src.entityData);
}

// This update function is only used with simple actors. We store the
// name used to initialise the actor in entity fields reserved for 
// save data that we know we won't use. Then we can use this name to
// recreate the simple actor on save state load.
void CreateSimpleActorDetourer::DetourCreateSimpleActor(char* name, DWORD type)
{
    mRealCreateSimpleActor((LPVOID)this, name, type);

    // Assuming none of the names passed to this function are on the heap.
    Entity((DWORD)this).GetSimpleActorSaveData() = (DWORD)name;
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
