#pragma once

#include <memory-wrapper.h>

class AswEngine : public MemoryWrapper
{
public:
    AswEngine(DWORD inPtr);
    DWORD& GetErrorCode();
    DWORD GetEntityCount() const;
    DWORD* GetEntityList();
    DWORD* GetPlayerList();
    class Entity GetP1Entity();
    Entity GetP2Entity();
    class ReplayHud GetReplayHud();
    class GameLogicManager GetGameLogicManager();

    // Many functions use this slight offset from the engine when
    // accessing it. It's possible this should be what we treat as 
    // the actual base address of the engine.
    DWORD GetOffset4();

    DWORD* GetPauseEngineUpdateFlag();
};

class GameLogicManager : public MemoryWrapper
{
public:
    GameLogicManager(DWORD inPtr);

    // This flag specifically stops the Asw engine from updating but does
    // not stop animations from updating. It also gets reset every frame 
    // based on whether the game is paused so we need to constantly set 
    // it if we want to prevent the engine state being updated. There are 
    // other pause flags which pause the game more completely but this one 
    // is better suited for what we want to do with replay scrubbing.
    DWORD& GetPauseEngineUpdateFlag();
};
