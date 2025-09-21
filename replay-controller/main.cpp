#include <replay-controller.h>
#include <detours.h>

class SetGameModeDetourer
{
public:
    void DetourSetGameMode(DWORD newMode);
    static SetGameModeFunc mRealSetGameMode; 
};
SetGameModeFunc SetGameModeDetourer::mRealSetGameMode = nullptr;

void SetGameModeDetourer::DetourSetGameMode(DWORD newMode)
{
    constexpr DWORD ReplayMode = 17;
    if (newMode == ReplayMode)
    {
        ReplayController::CreateInstance();
    }
    else if (ReplayController::IsActive())
    {
        ReplayController::DestroyInstance();
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

    // If we use device creation to find present on the game thread later it
    // breaks the game's presents. I'm not sure why but it probably interferes
    // with the existing D3D device somehow. So we initalise this on this thread,
    // otherwise would need to find present some other way.
    ReplayController::InitRealPresent();

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
