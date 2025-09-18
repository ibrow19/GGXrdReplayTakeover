#include <common.h>

char* GetModuleOffset(const char* name)
{
    if (name == nullptr)
    {
        MessageBox(NULL, "GetModuleOffset: no name provided", AppName, MB_OK);
    }

    HMODULE module = GetModuleHandleA(name);
    if(module == nullptr)
    {
        MessageBox(NULL, "GetModuleOffset: failed to get module handle", AppName, MB_OK);
        return nullptr;
    }

    HANDLE process = GetCurrentProcess();
    if (process == nullptr)
    {
        MessageBox(NULL, "GetModuleOffset: failed to get process handle", AppName, MB_OK);
        return nullptr;
    }

    MODULEINFO info;
    if (!GetModuleInformation(process, module, &info, sizeof(MODULEINFO)))
    {
        MessageBox(NULL, "GetModuleOffset: failed to get module info", AppName, MB_OK);
        return nullptr;
    }

    return (char*)(info.lpBaseOfDll);
}

// TODO: naming, we're not really getting the offset from this but rather the address
DWORD GetEngineOffset(const char* moduleOffset)
{
    DWORD* enginePtr = (DWORD*)(moduleOffset + 0x198b6e4);
    return *enginePtr;
}

void ForEachEntity(const std::function<void (DWORD)>& func)
{
    char* xrdOffset = GetModuleOffset(GameName);
    DWORD enginePtr = GetEngineOffset(xrdOffset);

    if (enginePtr == NULL)
    {
        return;
    }

    DWORD entityCount = *(DWORD*)(enginePtr + 0xb4);
    DWORD* entityList =  (DWORD*)(enginePtr + 0x1fc);

    for (DWORD i = 0; i < entityCount; ++i)
    {
        func(entityList[i]);
    }
}

void MakeRegionWritable(DWORD base, DWORD size)
{
    constexpr DWORD regionSize = 0x10000;

    DWORD end = base + size;
    DWORD extra = (DWORD)base & (regionSize - 1);
    DWORD pageStart = base - extra;

    while (pageStart < end)
    {
        DWORD oldPerms;
        BOOL bSuccess = VirtualProtect((LPVOID)base, regionSize, PAGE_EXECUTE_READWRITE, &oldPerms);
        if (!bSuccess)
        {
            LPVOID message;
            DWORD error = GetLastError();
            FormatMessage(
                    FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
                    NULL,
                    error,
                    MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                    (LPTSTR)&message,
                    0, NULL);
            MessageBox(NULL, (LPCTSTR)message, AppName, MB_OK);
        }
        pageStart += regionSize;
    }
}
