#pragma once

#include <memory-wrapper.h>

// Inputs bound from the "Button Settings(Menu)" menu
// The training mode Reset button is also part of this group but cannot be rebound
enum class MenuInputMask : DWORD
{
    Up = 0x1,
    Right = 0x2,
    Down = 0x4,
    Left = 0x8,
    AllDirections = Up | Down | Left | Right,

    Decide = 0x4000,
    Back = 0x8000,
    Menu = 0x20000,
    PageChange2 = 0x10000,

    Reset = 0x800000,
};

// Inputs bound from the "Button Settings(Battle)" menu
enum class BattleInputMask : DWORD
{
    Up = 0x1,
    Down = 0x2,
    Left = 0x4,
    Right = 0x8,
    AllDirections = Up | Down | Left | Right,

    P = 0x10,
    K = 0x20,
    S = 0x40,
    H = 0x80,
    D = 0x100,

    Taunt = 0x200,
    Special = 0x400,
    PK = 0x800,
    PKS = 0x1000,
    PKSH = 0x2000,
    SH = 0x4000,
    HD = 0x8000,

    PlayRecording = 0x10000,
    StartRecording = 0x20000,
    Pause = 0x40000,
};

// Contains input that has been transformed into a form ready to use in the
// game based on the chosen menu/battle bindings.
class GameInput : public MemoryWrapper
{
public:
    GameInput(DWORD inPtr);
    DWORD& GetPressedMask();
    DWORD& GetHeldMask();
};

class GameInputCollection : public MemoryWrapper
{
public:
    GameInputCollection(DWORD inPtr);
    GameInput GetP1MenuInput();
    GameInput GetP1BattleInput();
    GameInput GetP2MenuInput();
    GameInput GetP2BattleInput();
private:
    static constexpr DWORD PlayerOffset = 0x38;
    static constexpr DWORD BattleInputOffset = 0x54;
};
