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
typedef void(__thiscall* UpdateAnimationFunc)(DWORD entity, char* stateName, DWORD flag);
typedef void(__thiscall* SetGameModeFunc)(LPVOID thisArg, DWORD newMode);
typedef bool(__cdecl* IsPauseMenuActiveFunc)(void);
typedef void(__thiscall* SetHealthFunc)(LPVOID entity, int newHealth);
typedef void(__fastcall* UpdateTimeFunc)(DWORD timeData);
typedef void(__fastcall* HandleInputsFunc)(DWORD engine);

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
    static bool IsPauseMenuActive();

    // This flag is 1 during the match intro (heaven or hell section) and
    // exit (slash + spin around). It is 0 while a battle is in progress where 
    // players have control.
    static DWORD& GetPreOrPostBattle();

    // This function is called once per frame when playing online. It sets
    // specific fields on each entity, some of which are needed to recreate
    // the associated actor when loading a state on a rollback.
    static EntityUpdateFunc GetOnlineEntityUpdate();

    // Function including all input handling, entity and actor updates while
    // in a match in any offline game mode. While online a different function
    // is used.
    static MainGameLogicFunc GetMainGameLogic();

    // Setter for built in string type that use a fixed 32 byte buffer size.
    // The dest string is replaced with the src string then has the remaining 
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
    // rollback save state. PreLoad builds a list of references to actors
    // to re-attach to entities in the loaded state. This only applies to 
    // complex entities that can change based on user input like little Eddie. 
    // Player actors are not recreated as the rollback knows they'll still be 
    // there. Simple entities like projectiles aren't recreated as they'll 
    // either be using the same animation in the rolled back state or will
    // not have been created yet.
    static EntityActorManagementFunc GetPreStateLoad();
    static EntityActorManagementFunc GetPostStateLoad();

    // This function is called when a simple entity is created to initialise 
    // its associated actor. It is called again on some existing entities when
    // they need to change the animation they are displaying.
    static CreateActorFunc GetCreateSimpleActor();

    static DestroyActorFunc GetDestroySimpleActor();
    static DestroyActorFunc GetDestroyComplexActor();

    static UpdateAnimationFunc GetUpdateComplexActorAnimation();
    static SetGameModeFunc GetGameModeSetter();

    static SetHealthFunc GetSetHealth();
    static UpdateTimeFunc GetUpdateTime();
    static HandleInputsFunc GetHandleInputs();
private:
    static DWORD mBase;
};
