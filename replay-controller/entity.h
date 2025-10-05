#pragma once

#include <windows.h>

class Entity
{
public:
    Entity(DWORD inPtr);
    DWORD GetPtr();
    bool IsPlayer();
    DWORD& GetSimpleActor();
    DWORD& GetComplexActor();
    char* GetAnimationState();
private:
    DWORD mPtr;
};
