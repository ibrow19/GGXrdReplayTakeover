#include <xrd-module.h>
#include <replay-controller.h>
#include <training-controller.h>
#include <detours.h>

enum class GameMode : DWORD
{
    Training = 6,
    Replay = 17,
};

class SetGameModeDetourer
{
public:
    void DetourSetGameMode(DWORD newMode);
    static SetGameModeFunc mRealSetGameMode; 
};
SetGameModeFunc SetGameModeDetourer::mRealSetGameMode = nullptr;

void SetGameModeDetourer::DetourSetGameMode(DWORD newMode)
{
    DWORD modeByte = newMode&0xff;
    if (modeByte == (DWORD)GameMode::Replay)
    {
        GameModeController::Set<ReplayController>();
    }
    else if (modeByte == (DWORD)GameMode::Training)
    {
        GameModeController::Set<TrainingController>();
    }
    else
    {
        GameModeController::Destroy();
    }
    mRealSetGameMode(this, newMode);
}

void AttachSetGameModeDetour()
{
    SetGameModeDetourer::mRealSetGameMode = XrdModule::GetGameModeSetter();
    void (SetGameModeDetourer::* detourSetGameMode)(DWORD) = &SetGameModeDetourer::DetourSetGameMode;

    DetourTransactionBegin();
    DetourUpdateThread(GetCurrentThread());
    DetourAttach(&(PVOID&)SetGameModeDetourer::mRealSetGameMode, *(PBYTE*)&detourSetGameMode);
    DetourTransactionCommit();
}

extern "C" __declspec(dllexport) unsigned int RunInitThread(void*)
{
    XrdModule::Init();
#ifdef USE_IMGUI_OVERLAY
    GameModeController::InitD3DPresent();
#endif

    // Ideally we should do something to avoid race conditions when detouring
    // in this thread but I'm not sure the best way of doing this without a
    // more controlled way of injecting.
    AttachSetGameModeDetour();

    return 1;
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD reason, LPVOID lpReserved)
{
    return TRUE;
}
