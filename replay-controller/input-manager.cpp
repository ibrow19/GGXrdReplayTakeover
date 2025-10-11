#include <xrd-module.h>
#include <input-manager.h>

static constexpr DWORD P1ModeOffset = 0x1c;
static constexpr DWORD P2ModeOffset = P1ModeOffset + 0x24;
static constexpr DWORD P1ReplayPosOffset = 0x4;
static constexpr DWORD P2ReplayPosOffset = 0x28;

InputManager::InputManager(DWORD inPtr)
: mPtr(inPtr)
{}

InputMode InputManager::GetP1InputMode() const
{
    return *(InputMode*)(mPtr + P1ModeOffset);
}

InputMode InputManager::GetP2InputMode() const
{
    return *(InputMode*)(mPtr + P2ModeOffset);
}

ReplayPosition InputManager::GetP1ReplayPos() const
{
    ReplayPosition pos;
    memcpy(&pos.data, (void*)(mPtr + P1ReplayPosOffset), ReplayPosition::Size);
    return pos;
}

ReplayPosition InputManager::GetP2ReplayPos() const
{
    ReplayPosition pos;
    memcpy(&pos.data, (void*)(mPtr + P2ReplayPosOffset), ReplayPosition::Size);
    return pos;
}

void InputManager::SetP1InputMode(InputMode mode)
{
    *(DWORD*)(mPtr + P1ModeOffset) = (DWORD)mode;
}

void InputManager::SetP2InputMode(InputMode mode)
{
    *(DWORD*)(mPtr + P2ModeOffset) = (DWORD)mode;
}

void InputManager::SetP1ReplayPos(const ReplayPosition& pos)
{  
    memcpy((void*)(mPtr + P1ReplayPosOffset), &pos.data, ReplayPosition::Size);
}

void InputManager::SetP2ReplayPos(const ReplayPosition& pos)
{  
    memcpy((void*)(mPtr + P2ReplayPosOffset), &pos.data, ReplayPosition::Size);
}
