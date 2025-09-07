#include <common.h>
#include <input-manager.h>

void InputManager::Init()
{
    char* xrdOffset = GetModuleOffset(GameName);
    ptr = (DWORD)xrdOffset + InputDataOffset;
}

DWORD InputManager::GetP1InputMode() const
{
    return *(DWORD*)(ptr + P1ModeOffset);
}

DWORD InputManager::GetP2InputMode() const
{
    return *(DWORD*)(ptr + P2ModeOffset);
}

ReplayPosition InputManager::GetP1ReplayPos() const
{
    ReplayPosition pos;
    memcpy(&pos.data, (void*)(ptr + P1ReplayPosOffset), ReplayPosition::Size);
    return pos;
}

ReplayPosition InputManager::GetP2ReplayPos() const
{
    ReplayPosition pos;
    memcpy(&pos.data, (void*)(ptr + P2ReplayPosOffset), ReplayPosition::Size);
    return pos;
}

void InputManager::SetP1InputMode(InputMode mode)
{
    *(DWORD*)(ptr + P1ModeOffset) = (DWORD)mode;
}

void InputManager::SetP2InputMode(InputMode mode)
{
    *(DWORD*)(ptr + P2ModeOffset) = (DWORD)mode;
}

void InputManager::SetP1ReplayPos(const ReplayPosition& pos)
{  
    memcpy((void*)(ptr + P1ReplayPosOffset), &pos.data, ReplayPosition::Size);
}

void InputManager::SetP2ReplayPos(const ReplayPosition& pos)
{  
    memcpy((void*)(ptr + P2ReplayPosOffset), &pos.data, ReplayPosition::Size);
}
