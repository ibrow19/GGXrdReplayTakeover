#include <windows.h>
#include <imgui.h>
#include <imgui_impl_dx9.h>
#include <imgui_impl_win32.h>
#include <d3d9.h>

// TODO: link via cmake
#pragma comment (lib, "user32.lib")

void attach()
{
    MessageBox(NULL, "DLL successfully loaded", "Replay Controller", MB_OK);
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD reason, LPVOID lpReserved)
{
    switch (reason)
    {
        case DLL_PROCESS_ATTACH:
            attach();
            break;
        case DLL_PROCESS_DETACH:
            break;
        case DLL_THREAD_ATTACH:
            break;
        case DLL_THREAD_DETACH:
            break;
    }

    return TRUE;
}
