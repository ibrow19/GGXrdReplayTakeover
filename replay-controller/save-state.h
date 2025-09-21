#pragma once

#include <windows.h>

constexpr size_t SaveStateSize = 0x23C732;

struct SaveStateTracker
{
    DWORD padding0;
    DWORD saveStateIndex = 0;
    DWORD padding1;
    DWORD padding2;

    DWORD getTrackerAnchor;
    DWORD padding3;

    // +0x8
    DWORD saveAddress1 = 0;

    // +0xc
    DWORD saveMemCpyCount = 0;

    // +0x10
    DWORD loadMemCpyCount = 0;

    // +0x14
    DWORD loadAddress = 0;

    // +0x18
    DWORD saveAddress2 = 0;
};

enum class SaveStateSectionBase 
{
    Module,
    Engine
};

// Part of the save state data. Each section has a function for loading and saving, 
// each function's address is relative to the Xrd module. 
//
// Each section resides at the specific offset, either from the xrd module or the 
// currently used AswEngine object.
struct SaveStateSection
{
    DWORD saveFunctionOffset;
    DWORD loadFunctionOffset;

    SaveStateSectionBase base;
    DWORD offset;
};

void AttachSaveStateDetours();
void DetachSaveStateDetours();
void SaveState(char* dest);
void LoadState(const char* src);
