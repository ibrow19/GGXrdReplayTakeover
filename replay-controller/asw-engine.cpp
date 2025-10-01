#include <asw-engine.h>
#include <replay-hud.h>

AswEngine::AswEngine(DWORD inPtr)
: mPtr(inPtr)
{}

bool AswEngine::IsValid() const
{
    return mPtr != 0;
}

DWORD AswEngine::GetPtr() 
{
    return mPtr;
}

DWORD AswEngine::GetOffset4()
{
    return mPtr + 4;
}

DWORD AswEngine::GetEntityCount() const
{
    return *(DWORD*)(mPtr + 0xb4);
}

DWORD* AswEngine::GetEntityList()
{
    return (DWORD*)(mPtr + 0x1fc);
}

DWORD* AswEngine::GetPauseEngineUpdateFlag()
{
    // Not sure exactly what these objects are for other than they
    // contain a chain of references to the flag we want.
    DWORD object1 = *(DWORD*)(mPtr + 0x22e630);
    if (object1 == 0)
    {
        return nullptr;
    }

    DWORD object2 = *(DWORD*)(object1 + 0x37c);
    if (object2 == 0)
    {
        return nullptr;
    }

    return (DWORD*)(object2 + 0x1c8);
}

ReplayHud AswEngine::GetReplayHud()
{
    return ReplayHud(mPtr + 0x1c7400);
}
