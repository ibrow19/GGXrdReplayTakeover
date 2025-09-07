#pragma once

#include <windows.h>

class InputManager
{
public:
    enum class InputMode : DWORD
    {
        Player = 0,
        Replay = 2
    };

public:

    void Init();

    DWORD GetP1InputMode() const;
    DWORD GetP2InputMode() const;
    DWORD GetReplayPosition() const;

    void SetP1InputMode(InputMode mode);
    void SetP2InputMode(InputMode mode);
    void SetReplayPosition(DWORD pos);

private:

    static constexpr DWORD InputDataOffset = 0x1c77180 + 0x25380;
    static constexpr DWORD P1ModeOffset = 0x1c;
    static constexpr DWORD P2ModeOffset = P1ModeOffset + 0x24;
    static constexpr DWORD ReplayPositionOffset = 0x28;

private:

    DWORD ptr;
};
