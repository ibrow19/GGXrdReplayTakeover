#include <replay-controller.h>
#include <save-state.h>
#include <replay-manager.h>
#include <common.h>
#include <imgui.h>
#include <imgui_impl_dx9.h>
#include <imgui_impl_win32.h>
#include <detours.h>

typedef HRESULT(STDMETHODCALLTYPE* D3D9PresentFunc)(IDirect3DDevice9* device, CONST RECT* pSourceRect, CONST RECT* pDestRect, HWND hDestWindowOverride, CONST RGNDATA* pDirtyRegion);

ReplayController* ReplayController::mInstance = nullptr;

static HWND GWindow;
static WNDPROC GRealWndProc = nullptr;

static D3D9PresentFunc GRealPresent = nullptr;
static ReplayHudUpdateFunc GRealReplayHudUpdate = nullptr;

class MainGameLogicDetourer
{
public:
    void DetourMainGameLogic(DWORD param);
    static MainGameLogicFunc mRealMainGameLogic;
};
MainGameLogicFunc MainGameLogicDetourer::mRealMainGameLogic = nullptr;

//static void ApplyOnlineEntityUpdates()
//{
//    EntityUpdateFunc update = XrdModule::GetOnlineEntityUpdate();
//    ForEachEntity([&update](DWORD entity)
//        {
//            update(entity);
//        });
//}

void MainGameLogicDetourer::DetourMainGameLogic(DWORD param)
{
    //if (GbPaused)
    //{
    //    DWORD* pauseFlag = XrdModule::GetEngine().GetPauseEngineUpdateFlag();
    //    if (pauseFlag != nullptr)
    //    {
    //        *pauseFlag = 1;
    //    }
    //}

    //OnlineEntityUpdates();

    //if (GbRecording)
    //{
    //    if (GbPendingRetry)
    //    {
    //        GbPaused = false;
    //        GbRetrying = true;
    //        GSelectedFrame = GReplayManager.LoadFrame(GBookmark);
    //        InputMode p1Mode = GbControlP1 ? InputMode::Player : InputMode::Replay;
    //        InputMode p2Mode = GbControlP2 ? InputMode::Player : InputMode::Replay;
    //        GReplayManager.SetPlayerControl(p1Mode, p2Mode);
    //    }
    //    else if (GbPendingCancel && GbRetrying)
    //    {
    //        GbPaused = false;
    //        GbRetrying = false;
    //        GSelectedFrame = GReplayManager.LoadFrame(GBookmark);
    //        GReplayManager.ResetPlayerControl();
    //    }
    //    else if (!GbRetrying)
    //    {
    //        if (GbPaused)
    //        {
    //            if (GbPendingLoadFrame)
    //            {
    //                GSelectedFrame = GReplayManager.LoadFrame(GSelectedFrame);
    //            }
    //        }
    //        else
    //        {
    //            GSelectedFrame = GReplayManager.RecordFrame();
    //        }
    //    }
    //}
    //else if (GbPendingSave)
    //{
    //    SaveState(GSaveStateBuffer);
    //    GbStateSaved = true;
    //}
    //else if (GbPendingLoad)
    //{
    //    if (GbStateSaved)
    //    {
    //        LoadState(GSaveStateBuffer);
    //    }
    //}

    //GbPendingCancel = false;
    //GbPendingRetry = false;
    //GbPendingSave = false;
    //GbPendingLoad = false;

    mRealMainGameLogic((LPVOID)this, param);
}

void __fastcall DetourReplayHudUpdate(DWORD param)
{
    //if (!GbRetrying)
    //{
        GRealReplayHudUpdate(param);
    //}
}

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
static LRESULT CALLBACK ImGuiWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    if (ImGui_ImplWin32_WndProcHandler(hWnd, message, wParam, lParam))
    {
        return true;
    }

    return CallWindowProc(GRealWndProc, hWnd, message, wParam, lParam);
}

void ReplayController::CreateInstance()
{
    if (mInstance != nullptr)
    {
        return;
    }
    mInstance = new ReplayController;
}

void ReplayController::DestroyInstance()
{
    if (mInstance != nullptr)
    {
        delete mInstance;
        mInstance = nullptr;
    }
}

bool ReplayController::IsActive()
{
    return mInstance != nullptr;
}

ReplayController& ReplayController::Get()
{
    if (mInstance == nullptr)
    {
        CreateInstance();
    }
    return *mInstance;
}

ReplayController::ReplayController()
: mbImGuiInitialised(false)
{
    AttachDetours();
    AttachSaveStateDetours();
}

ReplayController::~ReplayController()
{
    // TODO: imgui shutdown is not safe while the render thread could use it. Need to flush render commands.
    DetachDetours();
    DetachSaveStateDetours();
    ShutdownImGui();
}

HRESULT STDMETHODCALLTYPE DetourPresent(IDirect3DDevice9* device, const RECT* pSourceRect, const RECT* pDestRect, HWND hDestWindowOverride, const RGNDATA* pDirtyRegion)
{
    // TODO: this access isn't thread safe while we're not flush game thread on shutdown
    ReplayController::Get().RenderUi(device);
    return GRealPresent(device, pSourceRect, pDestRect, hDestWindowOverride, pDirtyRegion);
}

void ReplayController::InitRealPresent()
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

void ReplayController::AttachDetours()
{
    // TODO: Flush render commands to prevent race condition while detouring render thread functions
    assert(GRealPresent);
    D3D9PresentFunc detourPresent = DetourPresent;
    GRealReplayHudUpdate = XrdModule::GetReplayHudUpdate();
    MainGameLogicDetourer::mRealMainGameLogic = XrdModule::GetMainGameLogic();
    void (MainGameLogicDetourer::* detourMainGameLogic)(DWORD) = &MainGameLogicDetourer::DetourMainGameLogic;

    DetourTransactionBegin();
    DetourUpdateThread(GetCurrentThread());
    DetourAttach(&(PVOID&)GRealPresent, (PVOID&)detourPresent);
    DetourAttach(&(PVOID&)GRealReplayHudUpdate, DetourReplayHudUpdate);
    DetourAttach(&(PVOID&)MainGameLogicDetourer::mRealMainGameLogic, *(PBYTE*)&detourMainGameLogic);
    DetourTransactionCommit();
}

void ReplayController::DetachDetours()
{
    // TODO: Flush render commands to prevent race condition from detoured render thread functions
    assert(GRealPresent);
    D3D9PresentFunc detourPresent = DetourPresent;
    GRealReplayHudUpdate = XrdModule::GetReplayHudUpdate();
    MainGameLogicDetourer::mRealMainGameLogic = XrdModule::GetMainGameLogic();
    void (MainGameLogicDetourer::* detourMainGameLogic)(DWORD) = &MainGameLogicDetourer::DetourMainGameLogic;

    DetourTransactionBegin();
    DetourUpdateThread(GetCurrentThread());
    DetourDetach(&(PVOID&)GRealPresent, (PVOID&)detourPresent);
    DetourDetach(&(PVOID&)GRealReplayHudUpdate, DetourReplayHudUpdate);
    DetourDetach(&(PVOID&)MainGameLogicDetourer::mRealMainGameLogic, *(PBYTE*)&detourMainGameLogic);
    DetourTransactionCommit();
}

void ReplayController::InitImGui(IDirect3DDevice9* device)
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

void ReplayController::ShutdownImGui()
{
    if (!mbImGuiInitialised)
    {
        return;
    }
    SetWindowLongPtr(GWindow, GWLP_WNDPROC, (LONG_PTR)GRealWndProc);
    ImGui_ImplDX9_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();
    mbImGuiInitialised = false;
}

void ReplayController::RenderUi(IDirect3DDevice9* device)
{
    if (!mbImGuiInitialised)
    {
        InitImGui(device);
    }
    
    PrepareImGuiFrame();
    device->BeginScene();
    ImGui::Render();
    ImGui_ImplDX9_RenderDrawData(ImGui::GetDrawData());
    device->EndScene();
}

void ReplayController::PrepareImGuiFrame()
{
    ImGui_ImplDX9_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();

    ImGui::Begin("Replay Takeover");

    //ImGui::Text("Save State");
    //if (ImGui::Button("Save") || ImGui::IsKeyPressed(ImGuiKey::ImGuiKey_F1))
    //{
    //    GbPendingSave = true;
    //}

    //ImGui::SameLine();
    //if (ImGui::Button("Load") || ImGui::IsKeyPressed(ImGuiKey::ImGuiKey_F2))
    //{
    //    GbPendingLoad = true;
    //}

    ImGui::Text("Replay Takeover");
    //if (ImGui::Button("Start") || ImGui::IsKeyPressed(ImGuiKey::ImGuiKey_F3))
    //{
    //    GbRecording = true;
    //}

    //ImGui::SameLine();
    //if (ImGui::Button("Stop") || ImGui::IsKeyPressed(ImGuiKey::ImGuiKey_F4))
    //{
    //    GbRecording = false;
    //    GReplayManager.Reset();
    //}

    //ImGui::SameLine();
    //if (ImGui::Button("Pause") || ImGui::IsKeyPressed(ImGuiKey::ImGuiKey_F5))
    //{
    //    GbPaused = !GbPaused;
    //}

    //if (ImGui::Button("Bookmark") || ImGui::IsKeyPressed(ImGuiKey::ImGuiKey_F6))
    //{
    //    GBookmark = GSelectedFrame; 
    //}

    //ImGui::SameLine();
    //if (ImGui::Button("Retry") || ImGui::IsKeyPressed(ImGuiKey::ImGuiKey_Space))
    //{
    //    GbPendingRetry = true;
    //}

    //ImGui::SameLine();
    //if (ImGui::Button("Cancel") || ImGui::IsKeyPressed(ImGuiKey::ImGuiKey_F8))
    //{
    //    GbPendingCancel = true;
    //}

    //ImGui::Checkbox("P1 Control", &GbControlP1);
    //ImGui::SameLine();
    //ImGui::Checkbox("P2 Control", &GbControlP2);

    //if (!GReplayManager.IsEmpty())
    //{
    //    if (ImGui::SliderInt("Frame", &GSelectedFrame, 0, GReplayManager.GetFrameCount() - 1))
    //    {
    //        GbPendingLoadFrame = true;
    //    }
    //    else if (ImGui::IsKeyPressed(ImGuiKey::ImGuiKey_LeftArrow))
    //    {
    //        GbPendingLoadFrame = true;
    //        GSelectedFrame = GReplayManager.GetCurrentFrame() - 1;
    //    }
    //    else if (ImGui::IsKeyPressed(ImGuiKey::ImGuiKey_RightArrow))
    //    {
    //        GbPendingLoadFrame = true;
    //        GSelectedFrame = GReplayManager.GetCurrentFrame() + 1;
    //    }
    //}

    ImGui::End();
    ImGui::EndFrame();
}

