#include <common.h>

char* GetModuleOffset(const char* name)
{
    if (name == nullptr)
    {
        MessageBox(NULL, "GetModuleOffset: no name provided", GAppName, MB_OK);
    }

    HMODULE module = GetModuleHandleA(name);
    if(module == nullptr)
    {
        MessageBox(NULL, "GetModuleOffset: failed to get module handle", GAppName, MB_OK);
        return nullptr;
    }

    HANDLE process = GetCurrentProcess();
    if (process == nullptr)
    {
        MessageBox(NULL, "GetModuleOffset: failed to get process handle", GAppName, MB_OK);
        return nullptr;
    }

    MODULEINFO info;
    if (!GetModuleInformation(process, module, &info, sizeof(MODULEINFO)))
    {
        MessageBox(NULL, "GetModuleOffset: failed to get module info", GAppName, MB_OK);
        return nullptr;
    }

    return (char*)(info.lpBaseOfDll);
}
