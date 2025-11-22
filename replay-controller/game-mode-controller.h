#pragma once

#ifdef USE_IMGUI_OVERLAY
#include <d3d9.h>
typedef HRESULT(STDMETHODCALLTYPE* D3D9PresentFunc)(IDirect3DDevice9* device, CONST RECT* pSourceRect, CONST RECT* pDestRect, HWND hDestWindowOverride, CONST RGNDATA* pDirtyRegion);
#endif

// Adds functionality to the current Xrd game mode. There should only be one 
// active controller at a time which is managed from this class. Each controller 
// attaches detours on initialisation and detaches them on destruction.
// GameModeControllers should only be created/destroyed on the game thread.
class GameModeController
{
public:
#ifdef USE_IMGUI_OVERLAY
    // Finds d3d present address to detour later. If we use device creation to find
    // present on the game thread it breaks the game's presents. I'm not sure
    // why but it probably interferes with the existing D3D device somehow. So
    // we we need to initalise this on the injection initialisation thread,
    // otherwise we need to find present some other way.
    static void InitD3DPresent();
#endif

    template <typename Controller>
    static void Set();
    template <typename Controller>
    static Controller* Get();

    static GameModeController* Get();
    static void Destroy();

#ifdef USE_IMGUI_OVERLAY
    void RenderUi(IDirect3DDevice9* device);
#endif
    virtual void Tick() = 0;
private:
    void Init();
    void Shutdown();

    virtual void InitMode() = 0;
    virtual void ShutdownMode() = 0;

#ifdef USE_IMGUI_OVERLAY
    static void InitImGui(IDirect3DDevice9* device);
    virtual void PrepareImGuiFrame() = 0;
#endif
private:
    static GameModeController* mInstance;
#ifdef USE_IMGUI_OVERLAY
    static bool mbImGuiInitialised;
#endif
};

template <typename Controller>
void GameModeController::Set()
{
    if (!Get<Controller>())
    {
        Destroy();
        mInstance = new Controller;
        mInstance->Init();
    }
}

template <typename Controller>
Controller* GameModeController::Get()
{
    return dynamic_cast<Controller*>(mInstance);
}
