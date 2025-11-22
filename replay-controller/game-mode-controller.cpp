#include <game-mode-controller.h>
#include <common.h>
#include <xrd-module.h>
#include <detours.h>
#include <cassert>
#ifdef USE_IMGUI_OVERLAY
#include <imgui.h>
#include <imgui_impl_dx9.h>
#include <imgui_impl_win32.h>
#endif

class GameModeControllerDetours
{
public:
    void DetourMainGameLogic(DWORD param);
    static MainGameLogicFunc mRealMainGameLogic;
};

GameModeController* GameModeController::mInstance = nullptr;
MainGameLogicFunc GameModeControllerDetours::mRealMainGameLogic = nullptr;

#ifdef USE_IMGUI_OVERLAY
bool GameModeController::mbImGuiInitialised = false;
D3D9PresentFunc GRealPresent = nullptr;
ReplayHudUpdateFunc GRealReplayHudUpdate = nullptr;
static WNDPROC GRealWndProc = nullptr;
static HWND GWindow = nullptr;

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
static LRESULT CALLBACK ImGuiWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    if (ImGui_ImplWin32_WndProcHandler(hWnd, message, wParam, lParam))
    {
        return true;
    }

    return CallWindowProc(GRealWndProc, hWnd, message, wParam, lParam);
}

HRESULT STDMETHODCALLTYPE DetourPresent(IDirect3DDevice9* device, const RECT* pSourceRect, const RECT* pDestRect, HWND hDestWindowOverride, const RGNDATA* pDirtyRegion)
{
    GameModeController* controller = GameModeController::Get();
    assert(controller != nullptr);
    controller->RenderUi(device);
    return GRealPresent(device, pSourceRect, pDestRect, hDestWindowOverride, pDirtyRegion);
}

void GameModeController::InitD3DPresent()
{
    // Create D3D device to find present address.
    IDirect3D9* dummyD3D = Direct3DCreate9(D3D_SDK_VERSION);
    if (!dummyD3D)
    {
        MessageBox(NULL, "Failed to create dummy D3D object", AppName, MB_OK);
        return;
    }

    WNDCLASSEX windowClass;
    ZeroMemory(&windowClass, sizeof(windowClass));
    windowClass.cbSize = sizeof(WNDCLASSEX);
    windowClass.lpfnWndProc = DefWindowProc;
    windowClass.lpszClassName = "window";

    if (!RegisterClassEx(&windowClass))
    {
        MessageBox(NULL, "Failed register dummy window class", AppName, MB_OK);
        dummyD3D->Release();
        return;
    }

    HWND dummyWindow = CreateWindow("window", "Initialising...", WS_VISIBLE, 0, 0, 100, 100, NULL, NULL, NULL, NULL);
    if (!dummyWindow)
    {
        MessageBox(NULL, "Failed to create dummy window", AppName, MB_OK);
        dummyD3D->Release();
        return;
    }

    IDirect3DDevice9* dummyDevice = nullptr;
    
    D3DDISPLAYMODE d3ddm;
    dummyD3D->GetAdapterDisplayMode(D3DADAPTER_DEFAULT, &d3ddm);

    // TODO: I'm not sure which params are required to make sure device creation doesn't fail
    D3DPRESENT_PARAMETERS d3dpp;
    ZeroMemory(&d3dpp, sizeof(d3dpp));

    d3dpp.BackBufferCount = 1;
    d3dpp.BackBufferFormat = d3ddm.Format;
    d3dpp.SwapEffect = D3DSWAPEFFECT_DISCARD;
    d3dpp.Windowed = true;
    d3dpp.EnableAutoDepthStencil = true;
    d3dpp.AutoDepthStencilFormat = D3DFMT_D16;
    d3dpp.FullScreen_RefreshRateInHz = D3DPRESENT_RATE_DEFAULT;
    d3dpp.PresentationInterval = D3DPRESENT_INTERVAL_IMMEDIATE;

    HRESULT result = dummyD3D->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL,
            dummyWindow, D3DCREATE_SOFTWARE_VERTEXPROCESSING, &d3dpp,
            &dummyDevice);
    if (result != D3D_OK)
    {
        MessageBox(NULL, "Failed to create dummy device", AppName, MB_OK);
        DestroyWindow(dummyWindow);
        dummyD3D->Release();
        return;
    }

    const int presentIndex = 17;
    DWORD_PTR* vtablePointer = (DWORD_PTR*)dummyDevice;
    DWORD_PTR* vtable = (DWORD_PTR*)vtablePointer[0];
    GRealPresent = (D3D9PresentFunc)vtable[presentIndex];

    dummyDevice->Release();
    DestroyWindow(dummyWindow);
    dummyD3D->Release();
}

void GameModeController::InitImGui(IDirect3DDevice9* device)
{
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    
    // Get window from present parameters and reassign WndProc.
    // TODO: can we assume swapChain 0 is fine for this? Not sure how to derive the correct value otherwise
    IDirect3DSwapChain9* swapChain = nullptr;
    device->GetSwapChain(0, &swapChain);
    assert(swapChain);
    
    D3DPRESENT_PARAMETERS presentParams;
    swapChain->GetPresentParameters(&presentParams);
    GWindow = presentParams.hDeviceWindow;
    GRealWndProc = (WNDPROC)SetWindowLongPtr(GWindow, GWLP_WNDPROC, (LONG_PTR)ImGuiWndProc);
    
    ImGui_ImplWin32_Init(GWindow);
    ImGui_ImplDX9_Init(device);

    mbImGuiInitialised = true;
}

void GameModeController::RenderUi(IDirect3DDevice9* device)
{
    if (!mbImGuiInitialised)
    {
        InitImGui(device);
    }
    
    ImGui_ImplDX9_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();
    PrepareImGuiFrame();
    ImGui::EndFrame();

    device->BeginScene();
    ImGui::Render();
    ImGui_ImplDX9_RenderDrawData(ImGui::GetDrawData());
    device->EndScene();
}
#endif

void GameModeControllerDetours::DetourMainGameLogic(DWORD param)
{
    GameModeController* controller = GameModeController::Get();
    assert(controller != nullptr);
    controller->Tick();
    mRealMainGameLogic((LPVOID)this, param);
}

GameModeController* GameModeController::Get()
{
    return mInstance;
}

void GameModeController::Destroy()
{
    if (mInstance != nullptr)
    {
        mInstance->Shutdown();
        delete mInstance;
        mInstance = nullptr;
    }
}

void GameModeController::Init()
{
#ifdef USE_IMGUI_OVERLAY
    // TODO: flush render command queue so that render thread detours are attached safely. 
    assert(GRealPresent);
    D3D9PresentFunc detourPresent = DetourPresent;
#endif
    GameModeControllerDetours::mRealMainGameLogic = XrdModule::GetMainGameLogic();
    void (GameModeControllerDetours::* detourMainGameLogic)(DWORD) = &GameModeControllerDetours::DetourMainGameLogic;

    DetourTransactionBegin();
    DetourUpdateThread(GetCurrentThread());
#ifdef USE_IMGUI_OVERLAY
    DetourAttach(&(PVOID&)GRealPresent, (PVOID&)detourPresent);
#endif
    DetourAttach(&(PVOID&)GameModeControllerDetours::mRealMainGameLogic, *(PBYTE*)&detourMainGameLogic);
    DetourTransactionCommit();

    InitMode();
}

void GameModeController::Shutdown()
{
#ifdef USE_IMGUI_OVERLAY
    // TODO: flush render command queue so that render thread detours are detached safely. 
    assert(GRealPresent);
    D3D9PresentFunc detourPresent = DetourPresent;
#endif
    void (GameModeControllerDetours::* detourMainGameLogic)(DWORD) = &GameModeControllerDetours::DetourMainGameLogic;

    DetourTransactionBegin();
    DetourUpdateThread(GetCurrentThread());
#ifdef USE_IMGUI_OVERLAY
    DetourDetach(&(PVOID&)GRealPresent, (PVOID&)detourPresent);
#endif
    DetourDetach(&(PVOID&)GameModeControllerDetours::mRealMainGameLogic, *(PBYTE*)&detourMainGameLogic);
    DetourTransactionCommit();

    ShutdownMode();
}
