#pragma once

struct SaveStateTransactionTracker
{
    // Number of memcpys that have been executed while saving or loading a state.
    DWORD MemCpyCount = 0;

    // Current address for saving/loading data. Set to save state location
    // initially then is incremented by size of each memcpy to keep track of
    // location for the next memcpy.
    DWORD Address = 0;  
};

enum SaveStateSectionBase 
{
    Module,
    AswEngine
};

// Part of the save state data. Each section has a function for loading and saving, 
// each function's address is relative to the Xrd module. 
//
// Each section has the specified Size in memory and resides at the specific
// offset, either from the xrd module or the currently used AswEngine object.
struct SaveStateSection
{
    DWORD SaveFunctionAddress;
    DWORD LoadFunctionAddress;

    SaveStateSectionBase Base;
    DWORD Offset;
};
