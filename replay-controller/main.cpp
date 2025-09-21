#include <common.h>
#include <xrd-module.h>
#include <asw-engine.h>
#include <save-state.h>
#include <replay-manager.h>
#include <imgui.h>
#include <imgui_impl_dx9.h>
#include <imgui_impl_win32.h>
#include <d3d9.h>
#include <detours.h>

typedef HRESULT(STDMETHODCALLTYPE* D3D9PresentFunc)(IDirect3DDevice9* pThis, CONST RECT* pSourceRect, CONST RECT* pDestRect, HWND hDestWindowOverride, CONST RGNDATA* pDirtyRegion);

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

static WNDPROC GRealWndProc = NULL;
static D3D9PresentFunc GRealPresent = NULL;

static bool GbImguiInitialised = false;
static bool GbPendingSave = false;
static bool GbPendingLoad = false;
static bool GbStateSaved = false;
static char GSaveStateBuffer[SaveStateSize];

static int GSelectedFrame = 0;
static ReplayManager GReplayManager;
static bool GbPaused = false;
static bool GbRecording = false;
static bool GbPendingLoadFrame = false;
static bool GbPendingRetry = false;
static bool GbPendingCancel = false;
static int GBookmark = 0;
static bool GbRetrying = false;
static bool GbControlP1 = false;
static bool GbControlP2 = false;

void PrepareRenderImgui()
{
    ImGui_ImplDX9_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();

    ImGui::Begin("Test window");

    ImGui::Text("Save State");
    if (ImGui::Button("Save") || ImGui::IsKeyPressed(ImGuiKey::ImGuiKey_F1))
    {
        GbPendingSave = true;
    }

    ImGui::SameLine();
    if (ImGui::Button("Load") || ImGui::IsKeyPressed(ImGuiKey::ImGuiKey_F2))
    {
        GbPendingLoad = true;
    }

    ImGui::Text("Replay Takeover");
    if (ImGui::Button("Start") || ImGui::IsKeyPressed(ImGuiKey::ImGuiKey_F3))
    {
        GbRecording = true;
    }

    ImGui::SameLine();
    if (ImGui::Button("Stop") || ImGui::IsKeyPressed(ImGuiKey::ImGuiKey_F4))
    {
        GbRecording = false;
        GReplayManager.Reset();
    }

    ImGui::SameLine();
    if (ImGui::Button("Pause") || ImGui::IsKeyPressed(ImGuiKey::ImGuiKey_F5))
    {
        GbPaused = !GbPaused;
    }

    if (ImGui::Button("Bookmark") || ImGui::IsKeyPressed(ImGuiKey::ImGuiKey_F6))
    {
        GBookmark = GSelectedFrame; 
    }

    ImGui::SameLine();
    if (ImGui::Button("Retry") || ImGui::IsKeyPressed(ImGuiKey::ImGuiKey_Space))
    {
        GbPendingRetry = true;
    }

    ImGui::SameLine();
    if (ImGui::Button("Cancel") || ImGui::IsKeyPressed(ImGuiKey::ImGuiKey_F8))
    {
        GbPendingCancel = true;
    }

    ImGui::Checkbox("P1 Control", &GbControlP1);
    ImGui::SameLine();
    ImGui::Checkbox("P2 Control", &GbControlP2);

    if (!GReplayManager.IsEmpty())
    {
        if (ImGui::SliderInt("Frame", &GSelectedFrame, 0, GReplayManager.GetFrameCount() - 1))
        {
            GbPendingLoadFrame = true;
        }
        else if (ImGui::IsKeyPressed(ImGuiKey::ImGuiKey_LeftArrow))
        {
            GbPendingLoadFrame = true;
            GSelectedFrame = GReplayManager.GetCurrentFrame() - 1;
        }
        else if (ImGui::IsKeyPressed(ImGuiKey::ImGuiKey_RightArrow))
        {
            GbPendingLoadFrame = true;
            GSelectedFrame = GReplayManager.GetCurrentFrame() + 1;
        }
    }

    ImGui::End();
    ImGui::EndFrame();
}

// Replacement WndProc that handles ImGui first.
LRESULT CALLBACK ImGuiWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    if (ImGui_ImplWin32_WndProcHandler(hWnd, message, wParam, lParam))
    {
        return true;
    }

    return CallWindowProc(GRealWndProc, hWnd, message, wParam, lParam);
}

HRESULT STDMETHODCALLTYPE DetourPresent(IDirect3DDevice9* pThis, const RECT* pSourceRect, const RECT* pDestRect, HWND hDestWindowOverride, const RGNDATA* pDirtyRegion)
{
    if (!GbImguiInitialised)
    {
        // Create Imgui Context.
        ImGui::CreateContext();
        ImGuiIO& io = ImGui::GetIO();
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

        // Get window from present parameters and reassign WndProc.
        // TODO: can we assume swapChain 0 is fine for this? Not sure what value we're supposed to use
        IDirect3DSwapChain9* swapChain = nullptr;
        pThis->GetSwapChain(0, &swapChain);

        assert(swapChain);

        // TODO: error handling.
        D3DPRESENT_PARAMETERS presentParams;
        swapChain->GetPresentParameters(&presentParams);

        HWND window = presentParams.hDeviceWindow;

        // Intercept WndProc to handle events with ImGui.
        GRealWndProc = (WNDPROC)SetWindowLongPtr(window, GWLP_WNDPROC, (LONG_PTR)ImGuiWndProc);

        ImGui_ImplWin32_Init(window);
        ImGui_ImplDX9_Init(pThis);

        GbImguiInitialised = true;
    }
    
    PrepareRenderImgui();

    pThis->BeginScene();

    ImGui::Render();
    ImGui_ImplDX9_RenderDrawData(ImGui::GetDrawData());

    pThis->EndScene();

    return GRealPresent(pThis, pSourceRect, pDestRect, hDestWindowOverride, pDirtyRegion);
}

void InitPresentDetour()
{
    // Create Dummy D3D to find present address.
    IDirect3D9* dummyD3D = Direct3DCreate9(D3D_SDK_VERSION);
    if (!dummyD3D)
    {
        MessageBox(NULL, "Failed to create dummy D3D object", AppName, MB_OK);
        return;
    }

    WNDCLASSEX windowClass;  // window class
    ZeroMemory(&windowClass, sizeof(windowClass));
    windowClass.cbSize    = sizeof(WNDCLASSEX);
    windowClass.lpfnWndProc   = DefWindowProc;
    windowClass.lpszClassName = "window";

    if (!RegisterClassEx(&windowClass))
    {
        return;
    }

    HWND dummyWindow = CreateWindow("window", "Initialising...", WS_VISIBLE, 0, 0, 100, 100, NULL, NULL, NULL, NULL);
    if (!dummyWindow)
    {
        char buffer[33];
        itoa(GetLastError(), buffer, 10);
        MessageBox(NULL, "Failed to create dummy window", AppName, MB_OK);
        MessageBox(NULL, buffer, AppName, MB_OK);
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
        char buffer[33];
        itoa(GetLastError(), buffer, 10);
        MessageBox(NULL, "Failed to create dummy device", AppName, MB_OK);
        MessageBox(NULL, buffer, AppName, MB_OK);
        return;
    }

    // TODO: can we do this without randomly indexing into memory?
    // Find present address.
    DWORD_PTR* vtablePointer = (DWORD_PTR*)dummyDevice;
    DWORD_PTR* vtable = (DWORD_PTR*)vtablePointer[0];
    GRealPresent = reinterpret_cast<D3D9PresentFunc>(vtable[17]);

    D3D9PresentFunc detourPresent = DetourPresent;

    // TODO: we should probably pause the render thread here while we apply the
    // detour to make sure this is safe. But we can worry about that later when
    // things are actually working.

    // Detour present.
    DetourTransactionBegin();
    DetourUpdateThread(GetCurrentThread());

    DetourAttach(&(PVOID&)GRealPresent, (PVOID&)detourPresent);

    DetourTransactionCommit();

    dummyDevice->Release();
    dummyD3D->Release();
    DestroyWindow(dummyWindow);
}

////////// Main Game Logic Detour

struct MainGameLogicDetourer
{
    void DetourMainGameLogic(DWORD param);
    static MainGameLogicFunc realMainGameLogic;
};

MainGameLogicFunc MainGameLogicDetourer::realMainGameLogic = nullptr;

void OnlineEntityUpdates()
{
    EntityUpdateFunc update = XrdModule::GetOnlineEntityUpdate();
    ForEachEntity([&update](DWORD entity)
        {
            update(entity);
        });
}

void MainGameLogicDetourer::DetourMainGameLogic(DWORD param)
{
    if (GbPaused)
    {
        DWORD* pauseFlag = XrdModule::GetEngine().GetPauseEngineUpdateFlag();
        if (pauseFlag != nullptr)
        {
            *pauseFlag = 1;
        }
    }

    OnlineEntityUpdates();

    if (GbRecording)
    {
        if (GbPendingRetry)
        {
            GbPaused = false;
            GbRetrying = true;
            GSelectedFrame = GReplayManager.LoadFrame(GBookmark);
            InputMode p1Mode = GbControlP1 ? InputMode::Player : InputMode::Replay;
            InputMode p2Mode = GbControlP2 ? InputMode::Player : InputMode::Replay;
            GReplayManager.SetPlayerControl(p1Mode, p2Mode);
        }
        else if (GbPendingCancel && GbRetrying)
        {
            GbPaused = false;
            GbRetrying = false;
            GSelectedFrame = GReplayManager.LoadFrame(GBookmark);
            GReplayManager.ResetPlayerControl();
        }
        else if (!GbRetrying)
        {
            if (GbPaused)
            {
                if (GbPendingLoadFrame)
                {
                    GSelectedFrame = GReplayManager.LoadFrame(GSelectedFrame);
                }
            }
            else
            {
                GSelectedFrame = GReplayManager.RecordFrame();
            }
        }
    }
    else if (GbPendingSave)
    {
        SaveState(GSaveStateBuffer);
        GbStateSaved = true;
    }
    else if (GbPendingLoad)
    {
        if (GbStateSaved)
        {
            LoadState(GSaveStateBuffer);
        }
    }

    GbPendingCancel = false;
    GbPendingRetry = false;
    GbPendingSave = false;
    GbPendingLoad = false;

    realMainGameLogic((LPVOID)this, param);
}

void InitMainGameLogicDetour()
{
    MainGameLogicDetourer::realMainGameLogic = XrdModule::GetMainGameLogic();
    void (MainGameLogicDetourer::* detourMainGameLogic)(DWORD) = &MainGameLogicDetourer::DetourMainGameLogic;

    DetourTransactionBegin();
    DetourUpdateThread(GetCurrentThread());
    DetourAttach(&(PVOID&)MainGameLogicDetourer::realMainGameLogic, *(PBYTE*)&detourMainGameLogic);
    DetourTransactionCommit();
}

////////// Replay HUD Detouring

static ReplayHudUpdateFunc GRealReplayHudUpdate = NULL;
void __fastcall DetourReplayHudUpdate(DWORD param)
{
    if (!GbRetrying)
    {
        GRealReplayHudUpdate(param);
    }
}

void InitReplayHudDetour()
{
    GRealReplayHudUpdate = XrdModule::GetReplayHudUpdate();

    DetourTransactionBegin();
    DetourUpdateThread(GetCurrentThread());
    DetourAttach(&(PVOID&)GRealReplayHudUpdate, DetourReplayHudUpdate);
    DetourTransactionCommit();
}

extern "C" __declspec(dllexport) unsigned int RunInitThread(void*)
{
    XrdModule::Init();
    InitPresentDetour();
    InitMainGameLogicDetour();
    AttachSaveStateDetours();
    InitReplayHudDetour();

    return 1;
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD reason, LPVOID lpReserved)
{
    return TRUE;
}
