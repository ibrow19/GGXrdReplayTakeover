#include <pause-menu.h>

PauseMenuButtonTable::PauseMenuButtonTable(DWORD inPtr)
: MemoryWrapper(inPtr)
{}

BYTE& PauseMenuButtonTable::GetFlag(PauseMenuMode mode, PauseMenuButton button)
{
    return *(BYTE*)(mPtr + (DWORD)button * (DWORD)PauseMenuMode::Count + (DWORD)mode);
}
