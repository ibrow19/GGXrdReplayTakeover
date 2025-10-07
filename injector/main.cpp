#include <iostream>
#include <windows.h>
#include <tlhelp32.h>

int FindProcess(const char* processName)
{
    HANDLE snapshot;
    snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, /*pid = */ 0);
    if (snapshot == INVALID_HANDLE_VALUE)
    {
        std::cout << "Invalid snapshot handle" << std::endl;
        return EXIT_FAILURE;
    }

    int pid = 0;
    PROCESSENTRY32 processEntry;
    processEntry.dwSize = sizeof(PROCESSENTRY32);
    BOOL result = Process32First(snapshot, &processEntry);
    while (result)
    {
        if (strcmp(processEntry.szExeFile, processName) == 0)
        {
            pid = processEntry.th32ProcessID;
            break;
        }
        result = Process32Next(snapshot, &processEntry);
    }

    return pid;
}

int InjectDll(int pid)
{

    HANDLE process = OpenProcess(PROCESS_ALL_ACCESS, /*bInheritHandle =*/ FALSE, DWORD(pid));
    if (process == NULL)
    {
        std::cout << "Failed to open process" << std::endl;
        CloseHandle(process);
        return EXIT_FAILURE;
    }

    LPVOID remoteBuffer = VirtualAllocEx(process, NULL, MAX_PATH, (MEM_RESERVE | MEM_COMMIT), PAGE_EXECUTE_READWRITE);
    if (remoteBuffer == NULL)
    {
        std::cout << "Failed to allocate space for dll in process" << std::endl;
        CloseHandle(process);
        return EXIT_FAILURE;
    }

    char path[MAX_PATH];
    GetCurrentDirectory(MAX_PATH, path);
#ifdef NDEBUG
    strcat(path, "\\GGXrdReplayTakeover.dll");
#else
    strcat(path, "\\build\\Debug\\GGXrdReplayTakeover.dll");
#endif

    BOOL result = WriteProcessMemory(process, remoteBuffer, path, MAX_PATH, NULL);
    if (!result)
    {
        std::cout << "Failed to copy dll path into process" << std::endl;
        CloseHandle(process);
        return EXIT_FAILURE;
    }

    HMODULE kernel = GetModuleHandle("Kernel32");
    VOID* loadFunction = GetProcAddress(kernel, "LoadLibraryA");

    if (loadFunction == NULL)
    {
        std::cout << "Failed to get load function address" << std::endl;
        CloseHandle(process);
        return EXIT_FAILURE;
    }

    HANDLE injecterThread = CreateRemoteThread(process, NULL, 0, (LPTHREAD_START_ROUTINE)loadFunction, remoteBuffer, 0, NULL);
    if (injecterThread == NULL)
    {
        std::cout << "Failed to create thread" << std::endl;
        return EXIT_FAILURE;
    }

    WaitForSingleObject(injecterThread, INFINITE);
    DWORD injectedBase;
    GetExitCodeThread(injecterThread, &injectedBase);

    HMODULE localModule = LoadLibrary("GGXrdReplayTakeover.dll");
    if (localModule == NULL)
    {
        std::cout << "Failed to get local version of injected module" << std::endl;
        return EXIT_FAILURE;
    }

    VOID* localInitFunction = GetProcAddress(localModule, "RunInitThread");
    if (localInitFunction == NULL)
    {
        std::cout << "Failed to find init thread function: " << GetLastError() << std::endl;
        return EXIT_FAILURE;
    }

    DWORD initOffset = (DWORD)localInitFunction - (DWORD)localModule;
    DWORD remoteAddress = injectedBase + initOffset;

    HANDLE initThread = CreateRemoteThread(process, NULL, 0, (LPTHREAD_START_ROUTINE)remoteAddress, NULL, 0, NULL);

    if (initThread == NULL)
    {
        std::cout << "Failed to create init thread" << std::endl;
        return EXIT_FAILURE;
    }
    else
    {
        WaitForSingleObject(initThread, INFINITE);
    }

    CloseHandle(process);
    std::cout << "Injection complete" << std::endl;
    return EXIT_SUCCESS;
}

int main(int argc, char** argv)
{
    std::cout << "Running Injector" << std::endl;
    const char* processName = "GuiltyGearXrd.exe";
    int pid = FindProcess(processName);

    if (pid)
    {
        std::cout << "found pid: " << pid << std::endl;
    }
    else
    {
        std::cout << "failed to find pid" << std::endl;
        return EXIT_FAILURE;
    }

    return InjectDll(pid);
}
