#pragma once

#include <windows.h>

// Wraps a pointer to a struct, class, or some other structure in the process
// memory to help with access to its members/functions. Uses DWORD instead of
// an actual pointer type as we need to do lots of arithmetic to access members
// which becomes tricky if it's a pointer.
class MemoryWrapper
{
public:
    MemoryWrapper(DWORD inPtr);
    DWORD GetPtr();
    bool IsValid() const;
protected:
    DWORD mPtr;
};
