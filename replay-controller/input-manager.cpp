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

DWORD InputManager::GetReplayPosition() const
{
    return *(DWORD*)(ptr + ReplayPositionOffset);
}

void InputManager::SetP1InputMode(InputMode mode)
{
    *(DWORD*)(ptr + P1ModeOffset) = (DWORD)mode;
}

void InputManager::SetP2InputMode(InputMode mode)
{
    *(DWORD*)(ptr + P2ModeOffset) = (DWORD)mode;
}

void InputManager::SetReplayPosition(DWORD pos)
{  
    *(DWORD*)(ptr + ReplayPositionOffset) = pos;
}
