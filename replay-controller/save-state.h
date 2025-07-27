#pragma once

struct SaveStateTracker
{
    DWORD padding[2];

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

enum SaveStateSectionBase 
{
    Module,
    AswEngine
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
