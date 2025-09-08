#pragma once

#include <windows.h>

struct ReplayPosition
{
    static constexpr size_t Size = 0x12;
    char data[Size];
};

enum class InputMode : DWORD
{
    Player = 0,
    Replay = 2
};


class InputManager
{
public:

    void Init();

    DWORD GetP1InputMode() const;
    DWORD GetP2InputMode() const;
    ReplayPosition GetP1ReplayPos() const;
    ReplayPosition GetP2ReplayPos() const;

    void SetP1InputMode(InputMode mode);
    void SetP2InputMode(InputMode mode);
    void SetP1ReplayPos(const ReplayPosition& pos);
    void SetP2ReplayPos(const ReplayPosition& pos);

private:

    static constexpr DWORD InputDataOffset = 0x1c77180 + 0x25380;
    static constexpr DWORD P1ModeOffset = 0x1c;
    static constexpr DWORD P2ModeOffset = P1ModeOffset + 0x24;
    static constexpr DWORD P1ReplayPosOffset = 0x4;
    static constexpr DWORD P2ReplayPosOffset = 0x28;

private:

    DWORD ptr;
};
