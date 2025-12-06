#include <asw-engine.h>
#include <entity.h>
#include <replay-hud.h>
#include <cassert>

AswEngine::AswEngine(DWORD inPtr)
: MemoryWrapper(inPtr)
{}

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

DWORD* AswEngine::GetPlayerList()
{
    return (DWORD*)(mPtr + 0xc8);
}

Entity AswEngine::GetP1Entity()
{
    return Entity(GetPlayerList()[0]);
}

Entity AswEngine::GetP2Entity()
{
    return Entity(GetPlayerList()[1]);
}

ReplayHud AswEngine::GetReplayHud()
{
    return ReplayHud(mPtr + 0x1c73fc);
}

GameLogicManager AswEngine::GetGameLogicManager()
{
    DWORD gameLogicPtr = *(DWORD*)(mPtr + 0x22e630);
    assert(gameLogicPtr);
    return GameLogicManager(gameLogicPtr);
}

DWORD& AswEngine::GetErrorCode()
{
    return *(DWORD*)(mPtr + 0x1c6f4c + 0x4);
}

GameLogicManager::GameLogicManager(DWORD inPtr)
: MemoryWrapper(inPtr)
{}

DWORD& GameLogicManager::GetPauseEngineUpdateFlag()
{
    DWORD pauseObject = *(DWORD*)(mPtr + 0x37c);
    assert(pauseObject);
    return *(DWORD*)(pauseObject + 0x1c8);
}
