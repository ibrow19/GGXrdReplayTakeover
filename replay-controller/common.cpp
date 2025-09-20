#include <common.h>
#include <xrd-module.h>
#include <asw-engine.h>

void ForEachEntity(const std::function<void (DWORD)>& func)
{
    AswEngine engine = XrdModule::GetEngine();
    if (!engine.IsValid())
    {
        return;
    }

    DWORD entityCount = engine.GetEntityCount();
    DWORD* entityList =  engine.GetEntityList();
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
