#pragma once

#include <d3d9.h>

typedef HRESULT(STDMETHODCALLTYPE* D3D9PresentFunc)(IDirect3DDevice9* device, CONST RECT* pSourceRect, CONST RECT* pDestRect, HWND hDestWindowOverride, CONST RGNDATA* pDirtyRegion);

// Adds functionality to the current Xrd game mode. There should only be one 
// active controller at a time which is managed from this class. Each controller 
// attaches detours on initialisation and detaches them on destruction.
// GameModeControllers should only be created/destroyed on the game thread.
class GameModeController
{
public:
    // Finds d3d present address to detour later. If we use device creation to find
    // present on the game thread it breaks the game's presents. I'm not sure
    // why but it probably interferes with the existing D3D device somehow. So
    // we we need to initalise this on the injection initialisation thread,
    // otherwise we need to find present some other way.
    static void Init();

    template <typename Controller>
    static void Set();
    template <typename Controller>
    static Controller* Get();

    static GameModeController* Get();
    static void Destroy();

    void RenderUi(IDirect3DDevice9* device);
    virtual void Tick() = 0;
private:
    static void AttachCommonDetours();
    static void DetachCommonDetours();
    static void InitImGui(IDirect3DDevice9* device);

    virtual void AttachModeDetours() = 0;
    virtual void DetachModeDetours() = 0;
    virtual void PrepareImGuiFrame() = 0;
private:
    static GameModeController* mInstance;
    static bool mbImGuiInitialised;
};

template <typename Controller>
void GameModeController::Set()
{
    if (!Get<Controller>())
    {
        Destroy();
        mInstance = new Controller;
        mInstance->AttachCommonDetours();
        mInstance->AttachModeDetours();
    }
}

template <typename Controller>
Controller* GameModeController::Get()
{
    return dynamic_cast<Controller*>(mInstance);
}
