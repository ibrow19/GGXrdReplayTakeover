#include <replay-controller.h>
#include <asw-engine.h>
#include <input.h>
#include <save-state.h>
#include <replay-manager.h>
#include <common.h>
#include <imgui.h>
#include <imgui_impl_dx9.h>
#include <imgui_impl_win32.h>
#include <detours.h>

typedef HRESULT(STDMETHODCALLTYPE* D3D9PresentFunc)(IDirect3DDevice9* device, CONST RECT* pSourceRect, CONST RECT* pDestRect, HWND hDestWindowOverride, CONST RGNDATA* pDirtyRegion);

ReplayController* ReplayController::mInstance = nullptr;
bool ReplayController::mbImGuiInitialised = false;

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

void MainGameLogicDetourer::DetourMainGameLogic(DWORD param)
{
    ReplayController::Get().Tick();
    mRealMainGameLogic((LPVOID)this, param);
}

void __fastcall DetourReplayHudUpdate(DWORD param)
{
    if (!ReplayController::Get().IsInReplayTakeoverMode())
    {
        GRealReplayHudUpdate(param);
    }
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
    // TODO: make sure accesses are safe
    //if (mInstance == nullptr)
    //{
    //    CreateInstance();
    //}
    return *mInstance;
}

ReplayController::ReplayController()
: mMode(ReplayTakeoverMode::Disabled)
, mbControlP1(true)
, mCountdownTotal(DefaultCountdown)
, mCountdown(0)
, mBookmarkFrame(0)
{
    AttachDetours();
    AttachSaveStateDetours();
}

ReplayController::~ReplayController()
{
    // TODO: imgui shutdown is not safe while the render thread could use it. Need to flush render commands.
    DetachDetours();
    DetachSaveStateDetours();
    //ShutdownImGui();
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

    ImGui::Text("Takeover Mode: ");
    ImGui::SameLine();
    switch (mMode)
    {
        case ReplayTakeoverMode::Disabled:
            ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(255, 0, 0, 255));
            ImGui::Text("DISABLED");
            ImGui::PopStyleColor();
            break;
        case ReplayTakeoverMode::StandbyPaused:
        case ReplayTakeoverMode::Standby:
            ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(200, 200, 0, 255));
            ImGui::Text("STANDBY");
            ImGui::PopStyleColor();
            break;
        case ReplayTakeoverMode::TakeoverCountdown:
            ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(0, 255, 100, 255));
            ImGui::Text("COUNTDOWN");
            ImGui::PopStyleColor();
            break;
        case ReplayTakeoverMode::TakeoverControl:
            ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(0, 255, 0, 255));
            ImGui::Text("TAKEOVER");
            ImGui::PopStyleColor();
            break;

    }

    int playerNum = mbControlP1 ? 1 : 2;
    ImGui::Text("Control: Player %d", playerNum);

    ImGui::SliderInt("Countdown Frames", &mCountdownTotal, MinCountdown, MaxCountdown);

    if (!mReplayManager.IsEmpty())
    {
        int selectedFrame = mReplayManager.GetCurrentFrame();
        ImGui::SliderInt("Replay Frame", &selectedFrame, 0, mReplayManager.GetFrameCount() - 1);
    }

    ImGui::End();
    ImGui::EndFrame();
}

void ReplayController::OverridePlayerControl()
{
    InputManager inputManager = XrdModule::GetInputManager();
    if (mbControlP1)
    {
        inputManager.SetP1InputMode(InputMode::Player);
        inputManager.SetP2InputMode(InputMode::Replay);
    }
    else
    {
        inputManager.SetP1InputMode(InputMode::Replay);
        inputManager.SetP2InputMode(InputMode::Player);
    }
}

void ReplayController::ResetPlayerControl()
{
    InputManager inputManager = XrdModule::GetInputManager();
    inputManager.SetP1InputMode(InputMode::Replay);
    inputManager.SetP2InputMode(InputMode::Replay);
}

void ReplayController::HandleDisabledMode()
{
    GameInputCollection input = XrdModule::GetGameInput();
    DWORD& menuInput = input.GetP1MenuInput().GetPressedMask();

    // Disable normal replay controls, enable replay takeover
    if (menuInput & (DWORD)MenuInputMask::Reset)
    {
        mMode = ReplayTakeoverMode::Standby;
        menuInput = 0;
    }
}

void ReplayController::HandleStandbyMode()
{
    GameInputCollection input = XrdModule::GetGameInput();
    DWORD& menuInput = input.GetP1MenuInput().GetPressedMask();
    DWORD& battleInput = input.GetP1BattleInput().GetPressedMask();

    // Return to regular replay mode
    if (menuInput & (DWORD)MenuInputMask::Reset)
    {
        ResetPlayerControl();
        menuInput = 0;
        mMode = ReplayTakeoverMode::Disabled;
        return;
    }

    // Initiate takeover
    if (battleInput & (DWORD)BattleInputMask::PlayRecording)
    {
        OverridePlayerControl();
        mBookmarkFrame = mReplayManager.GetCurrentFrame();
        mCountdown = 0;
        mMode = ReplayTakeoverMode::TakeoverCountdown;
        return;
    }

    // Toggle player controlled in takeover
    if (battleInput & (DWORD)BattleInputMask::K)
    {
        mbControlP1 = !mbControlP1;
    }

    // Replay scrubbing while paused.
    if (mMode == ReplayTakeoverMode::StandbyPaused)
    {
        if (battleInput & (DWORD)BattleInputMask::Left)
        {
            mReplayManager.LoadPreviousFrame();
        }
        else if (battleInput & (DWORD)BattleInputMask::Right)
        {
            mReplayManager.LoadNextFrame();
        }
        else if (battleInput & (DWORD)BattleInputMask::Down)
        {
            size_t currentFrame = mReplayManager.GetCurrentFrame();
            if (currentFrame <= PausedFrameJump)
            {
                mReplayManager.LoadFrame(0);
            }
            else
            {
                mReplayManager.LoadFrame(currentFrame - PausedFrameJump);
            }
        }
        else if (battleInput & (DWORD)BattleInputMask::Up)
        {
            size_t newFrame = mReplayManager.GetCurrentFrame() + PausedFrameJump;
            size_t maxFrame = mReplayManager.GetFrameCount() - 1;
            if (newFrame > maxFrame)
            {
                newFrame = maxFrame;
            }
            mReplayManager.LoadFrame(newFrame);
        }
    }

    // Toggle pause
    if (battleInput & (DWORD)BattleInputMask::P)
    {
        if (mMode == ReplayTakeoverMode::Standby)
        {
            mMode = ReplayTakeoverMode::StandbyPaused;
        }
        else
        {
            mMode = ReplayTakeoverMode::Standby;
        }
    }
}

void ReplayController::HandleTakeoverMode()
{
    GameInputCollection input = XrdModule::GetGameInput();
    DWORD& battleInput = input.GetP1BattleInput().GetPressedMask();

    // Restart takeover
    if (battleInput & (DWORD)BattleInputMask::PlayRecording)
    {
        mReplayManager.LoadFrame(mBookmarkFrame);
        mCountdown = 0;
        mMode = ReplayTakeoverMode::TakeoverCountdown;
        return;
    }

    // Cancel takeover
    if (battleInput & (DWORD)BattleInputMask::StartRecording)
    {
        mReplayManager.LoadFrame(mBookmarkFrame);
        ResetPlayerControl();
        mMode = ReplayTakeoverMode::StandbyPaused;
        return;
    }

    // Progress countdown
    if (mMode == ReplayTakeoverMode::TakeoverCountdown)
    {
        ++mCountdown;
        if (mCountdown >= mCountdownTotal)
        {
            mMode = ReplayTakeoverMode::TakeoverControl;
        }
    }
}

void ReplayController::Tick()
{
    if (XrdModule::GetPreOrPostBattle())
    {
        mReplayManager.Reset();
        if (mMode != ReplayTakeoverMode::Disabled)
        {
            mMode = ReplayTakeoverMode::Disabled;
            ResetPlayerControl();
        }
        return;
    }

    if (mMode == ReplayTakeoverMode::Disabled || mMode == ReplayTakeoverMode::Standby)
    {
        ApplySaveStateEntityUpdates();
        mReplayManager.RecordFrame();
    }

    switch (mMode)
    {
        case ReplayTakeoverMode::Disabled:
            HandleDisabledMode();
            break;
        case ReplayTakeoverMode::Standby:
        case ReplayTakeoverMode::StandbyPaused:
            HandleStandbyMode();
            break;
        case ReplayTakeoverMode::TakeoverCountdown:
        case ReplayTakeoverMode::TakeoverControl:
            HandleTakeoverMode();
            break;
    }

    if (mMode == ReplayTakeoverMode::StandbyPaused || mMode == ReplayTakeoverMode::TakeoverCountdown)
    {
        DWORD* pause = XrdModule::GetEngine().GetPauseEngineUpdateFlag();
        if (pause != nullptr)
        {
            *pause = 1;
        }
    }
}

bool ReplayController::IsInReplayTakeoverMode() const
{
    return mMode != ReplayTakeoverMode::Disabled;
}
