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
