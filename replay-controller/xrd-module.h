#pragma once

#include <windows.h>

typedef void(__thiscall* MainGameLogicFunc)(LPVOID thisArg, DWORD param);
typedef void(__thiscall* SetStringFunc)(char* dest, char* src);
typedef void(__fastcall* EntityUpdateFunc)(DWORD entity);
typedef void(__fastcall* ReplayHudUpdateFunc)(DWORD param);
typedef DWORD(__fastcall* GetSaveStateTrackerFunc)(DWORD manager);
typedef void(__fastcall* EntityActorManagementFunc)(DWORD engine);
typedef void(__thiscall* CreateActorFunc)(LPVOID thisArg, char* name, DWORD type);
typedef void(__fastcall* DestroyActorFunc)(DWORD entity);
typedef void(__thiscall* UpdatePlayerAnimFunc)(DWORD entity, char* stateName, DWORD flag);
typedef void(__thiscall* SetGameModeFunc)(LPVOID thisArg, DWORD newMode);

class XrdModule
{
public:
    static bool Init();
    static bool IsValid();
    static DWORD GetBase();
    static class AswEngine GetEngine();
    static class InputManager GetInputManager();
    static class GameInputCollection GetGameInput();
    static BYTE* GetControllerIndexInstruction();

    // This function is called once per frame when playing online. It sets
    // specific fields on each entity, some of which are needed to recreate
    // the associated actor when loading a state on a rollback.
    static EntityUpdateFunc GetOnlineEntityUpdate();

    // Function including all input handling, entity and actor updates while
    // in a match in any offline game mode. While online a different function
    // is used.
    static MainGameLogicFunc GetMainGameLogic();

    // Setter for built in string type that seems to use a fixed buffer size.
    // The dest string is replaced with he src string then has the remaining 
    // bytes in the buffer memset to 0.
    static SetStringFunc GetSetString();

    static ReplayHudUpdateFunc GetReplayHudUpdate();

    // This getter returns a pointer to a struct with various info about
    // the current state being saved/loaded from. Some save/load state
    // functions use this struct to determine where to save/load from.
    static GetSaveStateTrackerFunc GetSaveStateTrackerGetter();

    // Points at the location of the main block of save/load state data. This
    // is used to directly access the save state data by each save/load state
    // function which doesn't use the above getter function.
    static DWORD& GetSaveStateTrackerPtr();

    // These functions handle actor destruction/recreation when loading a
    // rollback save state. PreLoad destroys actors that won't necessarily be
    // in the loaded state and PostLoad recreates actors for new entities or
    // those that have changed state. This only applies to stateful entities
    // that can change based on user input like little Eddie. Player actors
    // are not recreated as the rollback knows they'll still be there. Simple
    // entities like projectiles aren't recreated as they'll either be using
    // the same animation in the rolled back state or will not have been
    // created yet.
    static EntityActorManagementFunc GetPreStateLoad();
    static EntityActorManagementFunc GetPostStateLoad();

    // This function is called when an entity is created to initialise its
    // associated actor. It is called again on some existing entities when
    // they transition to new state.
    static CreateActorFunc GetCreateEntityActor();
    static DestroyActorFunc GetDestroyEntityActor();

    static UpdatePlayerAnimFunc GetUpdatePlayerAnim();
    static SetGameModeFunc GetGameModeSetter();
private:
    static DWORD mBase;
};
