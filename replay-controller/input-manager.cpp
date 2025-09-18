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

    // TODO: a more robust way of managing this change is needed so we don't
    //       accidentally leave this instrucion altered.
    // If we set player 2 to player control then it will look for input
    // from a second controller. However, we want it to use the first
    // controller. So we change this instruction in the input handling
    // so that it pushes player 1 index onto the stack instead of player 2 when
    // it's about to call the function for getting controller inputs it's going
    // to add to the input buffer. Normally this instruction is Push ESI which
    // is 0 or 1. We replace it with Push EDX as we know (pretty sure) that EDX
    // is always 0 here in replays. However, EDX can be 1 in other modes such
    // as training so we need to make sure we change the instruction back once
    // we're done so that we don't break the other game modes.
    BYTE* instruction = (BYTE*)(GetModuleOffset(GameName) + 0x9e05f7);
    MakeRegionWritable((DWORD)instruction, 1);
    *instruction = mode == InputMode::Player ? 0x52 : 0x56;
}

void InputManager::SetP1ReplayPos(const ReplayPosition& pos)
{  
    memcpy((void*)(ptr + P1ReplayPosOffset), &pos.data, ReplayPosition::Size);
}

void InputManager::SetP2ReplayPos(const ReplayPosition& pos)
{  
    memcpy((void*)(ptr + P2ReplayPosOffset), &pos.data, ReplayPosition::Size);
}
