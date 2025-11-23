#include <memory-wrapper.h>

MemoryWrapper::MemoryWrapper(DWORD inPtr)
: mPtr(inPtr)
{}

DWORD MemoryWrapper::GetPtr()
{
    return mPtr;
}

bool MemoryWrapper::IsValid() const
{
    return mPtr != 0;
}
