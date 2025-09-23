#include <input.h>
#include <cassert>

GameInputCollection::GameInputCollection(DWORD inPtr)
: mPtr(inPtr)
{}

bool GameInputCollection::IsValid() const
{
    return mPtr != 0;
}

GameInput GameInputCollection::GetP1MenuInput()
{
    return GameInput(mPtr + PlayerOffset);
}

GameInput GameInputCollection::GetP1BattleInput()
{
    return GameInput(mPtr + BattleInputOffset);
}

GameInput GameInputCollection::GetP2MenuInput()
{
    return GameInput(mPtr + PlayerOffset * 2);
}

GameInput GameInputCollection::GetP2BattleInput()
{
    return GameInput(mPtr + BattleInputOffset + PlayerOffset);
}

GameInput::GameInput(DWORD inPtr)
: mPtr(inPtr)
{}

bool GameInput::IsValid() const
{
    return mPtr != 0;
}

DWORD& GameInput::GetPressedMask()
{
    return *(DWORD*)(mPtr + 0x10);
}

DWORD& GameInput::GetHeldMask()
{
    DWORD heldData = *(DWORD*)(mPtr + 0x8);
    assert(heldData != 0);
    return *(DWORD*)(heldData + 0xc);
}
