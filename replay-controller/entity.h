#pragma once

#include <windows.h>

class Entity
{
public:
    Entity(DWORD inPtr);
    DWORD GetPtr();
    bool IsPlayer();
    DWORD GetSimpleActor();
    DWORD GetComplexActor();
private:
    DWORD mPtr;
};
