#pragma once

#include <windows.h>

class Entity
{
public:
    Entity(DWORD inPtr);
    DWORD GetPtr();
    bool IsPlayer();
    char* GetCharacterCode();

    // Parameters only used with simple actors (Actors that just play a single
    // animation once or on repeat and have no additional state e.g projectiles).
    DWORD& GetSimpleActor();

    // Data only used for save states, usually only set while playing online.
    // As far as I can tell we don't need this for how we use save states so
    // can place are own data here.
    DWORD& GetSimpleActorSaveData();

    // Parameters only used with complex actors (Actors that have animation/state 
    // that can change depending on user input/events e.g players or little eddie)
    DWORD& GetComplexActor();
    char* GetStateName();
    char* GetAnimationFrameName();
    bool HasNonPlayerComplexActorOnline();
    // Not sure exactly what this is for but is only used to determine how to
    // recreate complex actors on state load as far as I can tell.
    DWORD& GetComplexActorRecreationFlag();
private:
    DWORD mPtr;
};

class SimpleActor
{
public:
    SimpleActor(DWORD inPtr);
    DWORD GetPtr();
    DWORD& GetTime();
private:
    DWORD mPtr;
};
