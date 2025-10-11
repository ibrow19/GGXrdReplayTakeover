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
    InputManager(DWORD inPtr);

    InputMode GetP1InputMode() const;
    InputMode GetP2InputMode() const;
    ReplayPosition GetP1ReplayPos() const;
    ReplayPosition GetP2ReplayPos() const;

    void SetP1InputMode(InputMode mode);
    void SetP2InputMode(InputMode mode);
    void SetP1ReplayPos(const ReplayPosition& pos);
    void SetP2ReplayPos(const ReplayPosition& pos);
private:
    DWORD mPtr;
};
