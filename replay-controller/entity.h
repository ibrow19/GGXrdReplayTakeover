#pragma once

#include <windows.h>

class Entity
{
public:
    Entity(DWORD inPtr);
    bool IsPlayer();

private:
    DWORD mPtr;
};
