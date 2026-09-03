#include <replay-hud.h>

ReplayHud::ReplayHud(DWORD inPtr)
: MemoryWrapper(inPtr)
{}

void ReplayHud::Reset()
{
    constexpr size_t dataSize = 0x30;
    memset((void*)mPtr, 0, dataSize);
    GetDisplayGameHud() = 1;
    GetDisplayReplayHud() = 1;
    GetDisplayInputHistory() = 1;
    GetCameraMode() = 0xffffffff;
}

DWORD& ReplayHud::GetPause()
{
    return *(DWORD*)(mPtr + 0x4);
}

DWORD& ReplayHud::GetShouldStepNextFrame()
{
    return *(DWORD*)(mPtr + 0x8);
}

DWORD& ReplayHud::GetDisplayGameHud()
{
    return *(DWORD*)(mPtr + 0xc);
}

DWORD& ReplayHud::GetDisplayReplayHud()
{
    return *(DWORD*)(mPtr + 0x10);
}

DWORD& ReplayHud::GetDisplayInputHistory()
{
    return *(DWORD*)(mPtr + 0x14);
}

DWORD& ReplayHud::GetGoToNextRound()
{
    return *(DWORD*)(mPtr + 0x1c);
}

DWORD& ReplayHud::GetUseSpecialCamera()
{
    return *(DWORD*)(mPtr + 0x20);
}

DWORD& ReplayHud::GetCameraUnavailable()
{
    return *(DWORD*)(mPtr + 0x24);
}

DWORD& ReplayHud::GetCameraPosition()
{
    return *(DWORD*)(mPtr + 0x28);
}

DWORD& ReplayHud::GetCameraTimer()
{
    return *(DWORD*)(mPtr + 0x2c);
}

DWORD& ReplayHud::GetCameraMode()
{
    return *(DWORD*)(mPtr + 0x30);
}
