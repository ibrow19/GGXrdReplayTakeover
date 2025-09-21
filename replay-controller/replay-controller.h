#pragma once

#include <windows.h>
#include <d3d9.h>
#include <xrd-module.h>

class ReplayController
{
public:
    static void CreateInstance();
    static void DestroyInstance();
    static bool IsActive();
    static ReplayController& Get();
    static void InitRealPresent();

    void RenderUi(IDirect3DDevice9* device);
    void Tick();
private:
    ReplayController();
    ~ReplayController();

    static void AttachDetours();
    static void DetachDetours();

    void InitImGui(IDirect3DDevice9* device);
    void ShutdownImGui();
    void PrepareImGuiFrame();
private:
    static ReplayController* mInstance;

    bool mbImGuiInitialised;
};
