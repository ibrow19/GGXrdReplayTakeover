#pragma once

#include <memory-wrapper.h>

class ReplayHud : public MemoryWrapper
{
public:
    ReplayHud(DWORD inPtr);
    void Reset();
    DWORD& GetPause();
    DWORD& GetShouldStepNextFrame();
    DWORD& GetDisplayGameHud();
    DWORD& GetDisplayReplayHud();
    DWORD& GetDisplayInputHistory();
    DWORD& GetGoToNextRound();
    DWORD& GetUseSpecialCamera();
    DWORD& GetCameraUnavailable();
    DWORD& GetCameraPosition();
    DWORD& GetCameraTimer();
    DWORD& GetCameraMode();
};
