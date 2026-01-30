#include <common.h>
#include <save-state.h>
#include <xrd-module.h>
#include <asw-engine.h>
#include <detours.h>
#include <cassert>
#include <entity.h>

DEFINE_PROFILING_CATEGORY(SaveState)
DEFINE_PROFILING_CATEGORY(LoadState)

class CreateSimpleActorDetourer
{
public:
    void DetourCreateSimpleActor(char* name, DWORD type);
    void DetourCreateBedmanSealActor(char* name);
    static CreateActorFunc mRealCreateSimpleActor; 
    static CreateBedmanSealActorFunc mRealCreateBedmanSealActor; 
};
CreateActorFunc CreateSimpleActorDetourer::mRealCreateSimpleActor = nullptr;
CreateBedmanSealActorFunc CreateSimpleActorDetourer::mRealCreateBedmanSealActor = nullptr;

static GetSaveStateTrackerFunc GRealGetSaveStateTracker = nullptr;
static bool GbStateDetourActive = false;
static SaveStateTracker GTracker;

static constexpr int SaveStateFunctionCount = 16;
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
    // 6 - Various UI elements copied from Engine->0x22e628->0x458
    // Objects such as negative penalty pop up and associated sounds are recreated
    // by the load function. In certain game states it is possible for the recereaion 
    // of some UI objects to introduce null UObjects which causes a crash on the 
    // next GC. Probably some assumptions based on using these load functions
    // for rollback are not true for how we're using them. I have not been able 
    // to pinpoint which UI element's creation causes the null object even when 
    // forcing GC every frame.
    //
    // If this save data is not present then the UI elements still work
    // mostly fine as far as I can tell; they just derive their state
    // from the engine/game state. Although removing it does result it some
    // artefacts like extra counter hit messages while reversing. But it 
    // should be safe to exclude this save state section until we are able 
    // to understand why it cause bad objects to be created that crash GC.
    // 
    // Size is approximately 8500 bytes but varies depending on game state.
    // TODO: reduce overall save state size to account for removal of this
    // section after its maximum size been investigated.
    //{
    //    0xc05cf0,
    //    0xc060a0,
    //    SaveStateSectionBase::Module,
    //    0x198b800 
    //},
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

                char* name = (char*)entity.GetSimpleActorSaveData();
                assert(name);
                if (!name)
                {
                    return;
                }

                DWORD type = entity.GetHasNonPlayerComplexActor();
                if (type == 0)
                {
                    CreateBedmanSealActorFunc createActor = XrdModule::GetCreateBedmanSealActor();
                    createActor((LPVOID)entityPtr, name);
                }
                else
                {
                    CreateActorFunc createActor = XrdModule::GetCreateSimpleActor();
                    createActor((LPVOID)entityPtr, name, type);
                }
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
            DWORD simpleActorType = 0;

            if (simpleActor)
            {
                namePtr = entity.GetSimpleActorSaveData();
                simpleActorType = entity.GetHasNonPlayerComplexActor();
                saveData[index].simpleActorTime = SimpleActor(simpleActor).GetTime();
            }

            EntityUpdateFunc update = XrdModule::GetOnlineEntityUpdate();
            update(entityPtr);

            // Restore name/type in entity as they will have been overwritten by the online entity update.
            if (simpleActor)
            {
                entity.GetSimpleActorSaveData() = namePtr;
                entity.GetHasNonPlayerComplexActor() = simpleActorType;
            }
        },
        entitySaveData);
}

void SaveState(SaveData& dest)
{
    SCOPE_COUNTER(SaveState)

    PreSaveEntityUpdates(dest.entityData);

    GTracker.saveMemCpyCount = 0;
    GTracker.saveAddress1 = (DWORD)dest.xrdData;
    GTracker.saveAddress2 = (DWORD)dest.xrdData;

    DWORD& trackerPtr = XrdModule::GetSaveStateTrackerPtr();
    trackerPtr = (DWORD)&GTracker;

    GbStateDetourActive = true;
#ifndef NDEBUG
    DWORD totalSize = 0;
    DWORD prevStart = (DWORD)dest.xrdData;
#endif
    for (int i = 0; i < SaveStateFunctionCount; ++i)
    {
        CallSaveStateFunction(
            SaveStateSections[i].saveFunctionOffset, 
            SaveStateSections[i].base, 
            SaveStateSections[i].offset);

#ifndef NDEBUG
        DWORD sectionSize = GTracker.saveAddress1 - prevStart;
        totalSize += sectionSize;
        prevStart = GTracker.saveAddress1;
        assert(totalSize <= XrdSaveStateSize);
#endif
    }


    // If we don't reset the save slot ptr then the game will immediately crash
    // trying to check it when exiting the current game.
    trackerPtr = NULL;
    GbStateDetourActive = false;
}

void LoadState(const SaveData& src)
{
    SCOPE_COUNTER(LoadState)

    ResetGameState();

    GTracker.loadMemCpyCount = 0;
    GTracker.loadAddress = (DWORD)src.xrdData;

    DWORD& trackerPtr = XrdModule::GetSaveStateTrackerPtr();
    trackerPtr = (DWORD)&GTracker;

    GbStateDetourActive = true;
#ifndef NDEBUG
    DWORD totalSize = 0;
    DWORD prevStart = (DWORD)src.xrdData;
#endif
    for (int i = 0; i < SaveStateFunctionCount; ++i)
    {
        CallSaveStateFunction(
            SaveStateSections[i].loadFunctionOffset, 
            SaveStateSections[i].base, 
            SaveStateSections[i].offset);

#ifndef NDEBUG
        DWORD sectionSize = GTracker.loadAddress - prevStart;
        totalSize += sectionSize;
        prevStart = GTracker.loadAddress;
        assert(totalSize <= XrdSaveStateSize);
#endif
    }
    GbStateDetourActive = false;
    trackerPtr = NULL;

    // Restore non-player complex actor flag to use in post load.
    DWORD actorTypes[MaxEntities];
    ForEachEntity([](DWORD entityPtr, DWORD index, void* extraData)
        {
            DWORD* types = (DWORD*)extraData;
            types[index] = 0;
            Entity entity(entityPtr);
            if (entity.GetSimpleActor())
            {
                types[index] = entity.GetHasNonPlayerComplexActor();
            }

            if (!entity.GetComplexActor() || entity.IsPlayer())
            {
                entity.GetHasNonPlayerComplexActor() = 0;
            }
            else
            {
                entity.GetHasNonPlayerComplexActor() = 1;
            }
        },
        actorTypes);

    CallPostLoad();

    // Restore Actor Type
    ForEachEntity([](DWORD entityPtr, DWORD index, void* extraData)
        {
            DWORD* types = (DWORD*)extraData;
            Entity(entityPtr).GetHasNonPlayerComplexActor() = types[index];
        },
        actorTypes);

    // Only recreate simple actors after post load as if they are present for
    // that the function will try to use the memory that we have repurposed for 
    // simple actor animation times
    RecreateSimpleActors();
    UpdateAnimations(src.entityData);
}

// This update function is only used with simple actors. We store the
// name used to initialise the actor in entity fields reserved for 
// save data that we know we won't use. Then we can use this name to
// recreate the simple actor on save state load.
void CreateSimpleActorDetourer::DetourCreateSimpleActor(char* name, DWORD type)
{
    mRealCreateSimpleActor((LPVOID)this, name, type);
    Entity entity((DWORD)this);
    // Assuming none of the names passed to this function are on the heap.
    entity.GetSimpleActorSaveData() = (DWORD)name;


    // Replace non-player complex actor flag with actor type so we know how 
    // to recreate this actor. We can programatically restore the flag before 
    // calling the post load function that needs it.
    entity.GetHasNonPlayerComplexActor() = type;
}

void CreateSimpleActorDetourer::DetourCreateBedmanSealActor(char* name)
{
    mRealCreateBedmanSealActor((LPVOID)this, name);
    Entity entity((DWORD)this);
    entity.GetSimpleActorSaveData() = (DWORD)name;

    // Use value of zero on this flag to indicate this is a bedman seal (or any
    // other actor that uses this function).
    entity.GetHasNonPlayerComplexActor() = 0;
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
    CreateSimpleActorDetourer::mRealCreateBedmanSealActor = XrdModule::GetCreateBedmanSealActor();
    void (CreateSimpleActorDetourer::* detourCreateSimpleActor)(char*, DWORD) = &CreateSimpleActorDetourer::DetourCreateSimpleActor;
    void (CreateSimpleActorDetourer::* detourCreateBedmanSealActor)(char*) = &CreateSimpleActorDetourer::DetourCreateBedmanSealActor;

    DetourTransactionBegin();
    DetourUpdateThread(GetCurrentThread());
    DetourAttach(&(PVOID&)GRealGetSaveStateTracker, DetourGetSaveStateTracker);
    DetourAttach(&(PVOID&)CreateSimpleActorDetourer::mRealCreateSimpleActor, *(PBYTE*)&detourCreateSimpleActor);
    DetourAttach(&(PVOID&)CreateSimpleActorDetourer::mRealCreateBedmanSealActor, *(PBYTE*)&detourCreateBedmanSealActor);
    DetourTransactionCommit();
}

void DetachSaveStateDetours()
{
    void (CreateSimpleActorDetourer::* detourCreateSimpleActor)(char*, DWORD) = &CreateSimpleActorDetourer::DetourCreateSimpleActor;
    void (CreateSimpleActorDetourer::* detourCreateBedmanSealActor)(char*) = &CreateSimpleActorDetourer::DetourCreateBedmanSealActor;

    DetourTransactionBegin();
    DetourUpdateThread(GetCurrentThread());
    DetourDetach(&(PVOID&)GRealGetSaveStateTracker, DetourGetSaveStateTracker);
    DetourDetach(&(PVOID&)CreateSimpleActorDetourer::mRealCreateSimpleActor, *(PBYTE*)&detourCreateSimpleActor);
    DetourDetach(&(PVOID&)CreateSimpleActorDetourer::mRealCreateBedmanSealActor, *(PBYTE*)&detourCreateBedmanSealActor);
    DetourTransactionCommit();
}
