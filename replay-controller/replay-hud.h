#pragma once

#include <windows.h>

class ReplayHud
{
public:
    ReplayHud(DWORD inPtr);
    void Reset();
    DWORD& GetPause();
    DWORD& GetShouldStepNextFrame();
    DWORD& GetDisplayGameHud();
    DWORD& GetDisplayReplayHud();
    DWORD& GetDisplayInputHistory();
    DWORD& GetUseSpecialCamera();
    DWORD& GetCameraPosition();
    DWORD& GetCameraTimer();
    DWORD& GetCameraMode();
private:
    DWORD mPtr;
};
