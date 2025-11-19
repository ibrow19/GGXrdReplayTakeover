#include <pause-menu.h>

PauseMenuButtonTable::PauseMenuButtonTable(DWORD inPtr)
: mPtr(inPtr)
{}

DWORD PauseMenuButtonTable::GetPtr() const
{
    return mPtr;
}

BYTE& PauseMenuButtonTable::GetFlag(PauseMenuMode mode, PauseMenuButton button)
{
    return *(BYTE*)(mPtr + (DWORD)button * (DWORD)PauseMenuMode::Count + (DWORD)mode);
}
