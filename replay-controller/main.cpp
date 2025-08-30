#include <common.h>
#include <save-state.h>
#include <replay-manager.h>
#include <imgui.h>
#include <imgui_impl_dx9.h>
#include <imgui_impl_win32.h>
#include <d3d9.h>
#include <detours.h>

typedef HRESULT(STDMETHODCALLTYPE* D3D9Present_t)(IDirect3DDevice9* pThis, CONST RECT* pSourceRect, CONST RECT* pDestRect, HWND hDestWindowOverride, CONST RGNDATA* pDirtyRegion);

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

static WNDPROC GRealWndProc = NULL;
static D3D9Present_t GRealPresent = NULL;

static bool GbImguiInitialised = false;
static bool GbPendingSave = false;
static bool GbPendingLoad = false;
static bool GbStateSaved = false;
static char GSaveStateBuffer[SaveStateSize];

static int GSelectedFrame = 0;
static ReplayManager GReplayManager;
static bool GbPaused = false;

void PrepareRenderImgui()
{
    ImGui_ImplDX9_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();

    ImGui::Begin("Test window");

    if (ImGui::Button("Save") || ImGui::IsKeyPressed(ImGuiKey::ImGuiKey_F1))
    {
        GbPendingSave = true;
    }
    if (ImGui::Button("Load") || ImGui::IsKeyPressed(ImGuiKey::ImGuiKey_F2))
    {
        GbPendingLoad = true;
    }

    if (ImGui::Button("Pause") || ImGui::IsKeyPressed(ImGuiKey::ImGuiKey_F4))
    {
        GbPaused = !GbPaused;
    }

    if (ImGui::Button("Start"))
    {
        GReplayManager.BeginRecording();
    }

    // TODO: move frame set to game loop.
    if (GReplayManager.IsRecordingComplete())
    {
        if (ImGui::SliderInt("Frame", &GSelectedFrame, 0, ReplayManager::FrameCount - 1))
        {
            GSelectedFrame = GReplayManager.SetFrame(GSelectedFrame);
        }
        else if (ImGui::IsKeyPressed(ImGuiKey::ImGuiKey_LeftArrow))
        {
            GSelectedFrame = GReplayManager.DecrementFrame();
        }
        else if (ImGui::IsKeyPressed(ImGuiKey::ImGuiKey_RightArrow))
        {
            GSelectedFrame = GReplayManager.IncrementFrame();
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
    GRealPresent = reinterpret_cast<D3D9Present_t>(vtable[17]);

    D3D9Present_t detourPresent = DetourPresent;

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

////////// Main Loop Detour

typedef void(__thiscall* MainLoop_t)(LPVOID thisArg, DWORD param);

class MainLoopDetourer
{
public:
    void DetourMainLoop(DWORD param);

    static MainLoop_t realMainLoop;
};

MainLoop_t MainLoopDetourer::realMainLoop = nullptr;

typedef void(__fastcall* EntityUpdate_t)(DWORD entity);
void OnlineEntityUpdates()
{
    char* xrdOffset = GetModuleOffset(GameName);
    DWORD onlineEntityUpdateOffset = 0xb6efd0;
    EntityUpdate_t onlineEntityUpdate = (EntityUpdate_t)(xrdOffset + onlineEntityUpdateOffset);

    // TODO: make sure this isn't doing some unecessary allocation or something.
    ForEachEntity([&onlineEntityUpdate](DWORD entity)
        {
            onlineEntityUpdate(entity);
        });
}

void MainLoopDetourer::DetourMainLoop(DWORD param)
{
    if (GbPaused)
    {
        char* xrdOffset = GetModuleOffset(GameName);
        DWORD engine = GetEngineOffset(xrdOffset);
        DWORD offset1 = *(DWORD*)(engine + 0x22e630);
        DWORD offset2 = *(DWORD*)(offset1 + 0x37c);
        DWORD* pauseFlag = (DWORD*)(offset2 + 0x1c8);
        *pauseFlag = 1;
    }

    OnlineEntityUpdates();

    if (GReplayManager.IsRecording())
    {
        GReplayManager.RecordFrame();
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

    GbPendingSave = false;
    GbPendingLoad = false;

    realMainLoop((LPVOID)this, param);
}

// TODO: naming of detour and init functions to be consistent suffix/prefix
void InitMainLoopDetour()
{
    DWORD mainLoopOffset = 0xa61240;
    char* xrdOffset = GetModuleOffset(GameName);
    char* mainLoopAddress = xrdOffset + mainLoopOffset;

    MainLoopDetourer::realMainLoop = (MainLoop_t)mainLoopAddress;
    void (MainLoopDetourer::* detourMainLoop)(DWORD) = &MainLoopDetourer::DetourMainLoop;

    DetourTransactionBegin();
    DetourUpdateThread(GetCurrentThread());
    DetourAttach(&(PVOID&)MainLoopDetourer::realMainLoop, *(PBYTE*)&detourMainLoop);
    DetourTransactionCommit();
}

////////// Entity Update Detour

EntityUpdate_t GRealEntityUpdate = NULL;

void __fastcall DetourEntityUpdate(DWORD entity)
{
    char* xrdOffset = GetModuleOffset(GameName);
    DWORD onlineEntityUpdateOffset = 0xb6efd0;
    EntityUpdate_t onlineEntityUpdate = (EntityUpdate_t)(xrdOffset + onlineEntityUpdateOffset);
    onlineEntityUpdate(entity);

    GRealEntityUpdate(entity);
}

void InitEntityUpdateDetour()
{
    char* xrdOffset = GetModuleOffset(GameName);

    DWORD entityUpdateOffset = 0x9f7950;
    char* entityUpdateAddress = xrdOffset + entityUpdateOffset;

    GRealEntityUpdate = (EntityUpdate_t)entityUpdateAddress;

    DetourTransactionBegin();
    DetourUpdateThread(GetCurrentThread());
    DetourAttach(&(PVOID&)GRealEntityUpdate, DetourEntityUpdate);
    DetourTransactionCommit();
}

////////// Entity Init Detouring

typedef void(__thiscall* EntityInit_t)(LPVOID thisArg, DWORD name, DWORD type);

class EntityInitDetourer
{
public:
    void DetourEntityInit(DWORD name, DWORD type);

    static EntityInit_t realEntityInit;
};

EntityInit_t EntityInitDetourer::realEntityInit = nullptr;


 // This update function is only used with simple entities. We can use
 // entity fields reserved for more complicated, stateful entities to hold
 // initalisation data to recreate these entities later. 
void EntityInitDetourer::DetourEntityInit(DWORD name, DWORD type)
{
    bool bRecreatingSimple = *(DWORD*)((DWORD)this + 0x27cc) != 0 && *(DWORD*)((DWORD)this + 0x2878) == 0;

    realEntityInit((LPVOID)this, name, type);

    // Don't overwrite these values if the entity needs them.
    // Ignore entities where the simple actor (27cc) is already set as that
    // means we're resuing an existing simple actor and need to reset our custom changes.
    if (!bRecreatingSimple &&
            (*(DWORD*)((DWORD)this + 0x2878) != 0 ||
             *(DWORD*)((DWORD)this + 0x2858) != 0 ||
             *(DWORD*)((DWORD)this + 0x287c) != 0))
    {
        return;
    }

    // TODO: refactor all of this so setting/getting these particular flags
    // doesn't involve any redefinition of addresses etc.
    typedef void(__thiscall* SetString_t)(DWORD dest, DWORD src);

    char* xrdOffset = GetModuleOffset(GameName);
    DWORD setStringOffset = 0x973390;
    SetString_t setStringFunc = (SetString_t)(xrdOffset + setStringOffset);

    DWORD stateNamePtr = (DWORD)this + 0x2858;
    setStringFunc(stateNamePtr, name);

    *(DWORD*)((DWORD)this + 0x287c) = type;
}

void InitEntityInitDetour()
{
    DWORD entityInitOffset = 0xa0e020;
    char* xrdOffset = GetModuleOffset(GameName);
    char* entityInitAddress = xrdOffset + entityInitOffset;

    EntityInitDetourer::realEntityInit = (EntityInit_t)entityInitAddress;
    void (EntityInitDetourer::* detourEntityInit)(DWORD, DWORD) = &EntityInitDetourer::DetourEntityInit;

    DetourTransactionBegin();
    DetourUpdateThread(GetCurrentThread());
    DetourAttach(&(PVOID&)EntityInitDetourer::realEntityInit, *(PBYTE*)&detourEntityInit);
    DetourTransactionCommit();
}

extern "C" __declspec(dllexport) unsigned int RunInitThread(void*)
{
    // TODO: Divide these into other files where appropriate
    InitPresentDetour();
    InitMainLoopDetour();
    //InitEntityUpdateDetour();
    InitEntityInitDetour();
    InitSaveStateTrackerDetour();

    return 1;
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD reason, LPVOID lpReserved)
{
    return TRUE;
}
