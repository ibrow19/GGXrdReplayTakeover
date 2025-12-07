#pragma once

#include <windows.h>

typedef void(__thiscall* MainGameLogicFunc)(LPVOID thisArg, DWORD param);
typedef void(__thiscall* SetStringFunc)(char* dest, char* src);
typedef void(__fastcall* EntityUpdateFunc)(DWORD entity);
typedef void(__fastcall* ReplayHudUpdateFunc)(DWORD param);
typedef void(__thiscall* DisplayReplayHudMenuFunc)(LPVOID replayHud);
typedef void(__cdecl* AddUiTextFunc)(DWORD* textParams, DWORD param1, DWORD param2, DWORD param3, DWORD param4, DWORD param5);
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
typedef void(__thiscall* TickActorFunc)(LPVOID actor, float delta);
typedef bool(__cdecl* CheckInBattleFunc)(void);
typedef void(__cdecl* TickRelevantActorsFunc)(void);
typedef bool(__fastcall* IsResimulatingFunc)(DWORD rollbackManager);
typedef void(__cdecl* PlayBurstMaxSoundFunc)(void);
typedef void(__fastcall* DestroyAllSimpleActorsFunc)(DWORD gameLogicManager);
typedef void(__thiscall* ResetGameFunc)(DWORD engine, DWORD param);

class XrdModule
{
public:
    static bool Init();
    static bool IsValid();
    static DWORD GetBase();
    static class AswEngine GetEngine();
    static class InputManager GetInputManager();
    static class GameInputCollection GetGameInput();
    static class PauseMenuButtonTable GetPauseMenuButtonTable();
    static BYTE* GetControllerIndexInstruction();
    static BYTE* GetSoundEffectJumpInstruction();
    static BYTE* GetInputDisplayInstruction();
    static bool IsPauseMenuActive();
    static DWORD GetUiStringTable();
    static float GetReplayTextSpacing();
    static float& GetDefaultTickDelta();

    // Values modified from the pause menu. These are part of other structs
    // with various training/UI settings but we only need these specific
    // settings for the mod.
    static DWORD& GetTrainingP1MaxHealth();
    static DWORD& GetButtonDisplayMode();

    // This flag is 1 during the match intro (heaven or hell section) and
    // exit (slash + spin around). It is 0 while a battle is in progress where 
    // players have control.
    static DWORD& GetPreOrPostBattle();
    // This function should be approximately the inverse of checking the above flag
    // but is used in some different places in xrd's code. Might vary in some circumstances.
    static bool CheckInBattle();

    // This function is called once per frame when playing online. It sets
    // specific fields on each entity, some of which are needed to recreate
    // the associated actor when loading a state on a rollback.
    static EntityUpdateFunc GetOnlineEntityUpdate();

    // Function including all input handling, entity and actor updates while
    // in a match in any offline game mode. While online a different function
    // is used.
    static MainGameLogicFunc GetMainGameLogic();
    static MainGameLogicFunc GetOfflineMainGameLogic();

    // Setter for built in string type that use a fixed 32 byte buffer size.
    // The dest string is replaced with the src string then has the remaining 
    // bytes in the buffer memset to 0.
    static SetStringFunc GetSetString();

    static ReplayHudUpdateFunc GetReplayHudUpdate();
    static DisplayReplayHudMenuFunc GetDisplayReplayHudMenu();
    static AddUiTextFunc GetAddUiText();

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
    static TickActorFunc GetTickActor();
    static TickActorFunc GetTickSimpleActor();

    // This function is used to tick actors online when resimulating for
    // rollbacks. A "relevant" actor is one where *(Actor + 0x34) == 0x11166000
    // The value compared against is stored at Xrd+0x16f1084
    static TickRelevantActorsFunc GetTickRelevantActors();

    static IsResimulatingFunc GetIsResimulating();
    static PlayBurstMaxSoundFunc GetPlayBurstMaxSound();

    // Removes all simple actors being used in a game, including those not
    // directly attached to an entity e.g hitsparks. Possibly also stops 
    // any currently playing sound effects.
    static DestroyAllSimpleActorsFunc GetDestroyAllSimpleActors();

    static ResetGameFunc GetResetGame();
private:
    static DWORD mBase;
};
