#include <iostream>
#include <windows.h>
#include <tlhelp32.h>

#define TEST_DEFINE 262144.0

int findProcess(const char* processName)
{
    HANDLE snapshot;

    // snapshot all processes in the system.
    snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, /*pid = */ 0);
    if (snapshot == INVALID_HANDLE_VALUE)
    {
        std::cout << "Invalid snapshot handle" << std::endl;
        return EXIT_FAILURE;
    }
    std::cout << "valid snapshot" << std::endl;

    int pid = 0;

    PROCESSENTRY32 processEntry;
    processEntry.dwSize = sizeof(PROCESSENTRY32);

    // Info about first process in the snapshotted list
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

void injectDll(int pid)
{
    // TODO: update path once project structure more developed.
    const char dllPath[] = "GGXrdReplayController.dll";

    const unsigned int dllLen = sizeof(dllPath);

    // TODO: error handling. with GetLastError.
    HANDLE process = OpenProcess(PROCESS_ALL_ACCESS, /*bInheritHandle =*/ FALSE, DWORD(pid));

    // TODO: why do all the windows docs still use NULL in the year 2025
    if (process == NULL)
    {
        std::cout << "Failed to open process" << std::endl;
        CloseHandle(process);
        return;
    }

    // Allocate memory in process for dll.
    LPVOID remoteBuffer = VirtualAllocEx(process, NULL, dllLen, (MEM_RESERVE | MEM_COMMIT), PAGE_EXECUTE_READWRITE);

    if (remoteBuffer == NULL)
    {
        std::cout << "Failed to allocate space for dll in process" << std::endl;
        CloseHandle(process);
        return;
    }

    // Copy dll to the allocated buffer
    BOOL result = WriteProcessMemory(process, remoteBuffer, dllPath, dllLen, NULL);
    std::cout << "dllLen: " << dllLen << std::endl;

    if (!result)
    {
        std::cout << "Failed to copy dll path into process" << std::endl;
        CloseHandle(process);
        return;
    }

    // TODO: error handling for this.
    HMODULE kernel = GetModuleHandle("Kernel32");
    VOID* loadFunction = GetProcAddress(kernel, "LoadLibraryA");

    if (loadFunction == NULL)
    {
        std::cout << "Failed to get load function address" << std::endl;
        CloseHandle(process);
        return;
    }

    HANDLE injecterThread = CreateRemoteThread(process, NULL, 0, (LPTHREAD_START_ROUTINE)loadFunction, remoteBuffer, 0, NULL);
    if (injecterThread == NULL)
    {
        std::cout << "Failed to create thread" << std::endl;
        return;
    }

    WaitForSingleObject(injecterThread, INFINITE);
    DWORD injectedBase;
    GetExitCodeThread(injecterThread, &injectedBase);

    // TODO: better way of getting function offset.
    HMODULE localModule = LoadLibrary("GGXrdReplayController.dll");
    if (localModule == NULL)
    {
        std::cout << "Failed to get local version of injected module" << std::endl;
        return;
    }

    VOID* localInitFunction = GetProcAddress(localModule, "RunInitThread");
    if (localInitFunction == NULL)
    {
        std::cout << "Failed to find init thread function: " << GetLastError() << std::endl;
        return;
    }

    DWORD initOffset = (DWORD)localInitFunction - (DWORD)localModule;
    DWORD remoteAddress = injectedBase + initOffset;

    HANDLE initThread = CreateRemoteThread(process, NULL, 0, (LPTHREAD_START_ROUTINE)remoteAddress, NULL, 0, NULL);

    if (initThread == NULL)
    {
        std::cout << "Failed to create init thread" << std::endl;
    }
    else
    {
        std::cout << "waiting for init thread" << GetLastError() << std::endl;
        WaitForSingleObject(initThread, INFINITE);
        std::cout << "Finished waiting for init thread" << GetLastError() << std::endl;
    }

    CloseHandle(process);
}

int main(int argc, char** argv)
{
    std::cout << "Running" << std::endl;
    float test2 = TEST_DEFINE * TEST_DEFINE;
    float test3 = TEST_DEFINE * TEST_DEFINE* TEST_DEFINE;
    float test4 = TEST_DEFINE * TEST_DEFINE* TEST_DEFINE* TEST_DEFINE;
    float test5 = TEST_DEFINE * TEST_DEFINE* TEST_DEFINE* TEST_DEFINE* TEST_DEFINE;
    float test6 = TEST_DEFINE * TEST_DEFINE* TEST_DEFINE* TEST_DEFINE* TEST_DEFINE* TEST_DEFINE;
    float test7 = TEST_DEFINE * TEST_DEFINE* TEST_DEFINE* TEST_DEFINE* TEST_DEFINE* TEST_DEFINE* TEST_DEFINE;
    float test8 = TEST_DEFINE * TEST_DEFINE* TEST_DEFINE* TEST_DEFINE* TEST_DEFINE* TEST_DEFINE* TEST_DEFINE* TEST_DEFINE;
    float test9 = TEST_DEFINE * TEST_DEFINE* TEST_DEFINE* TEST_DEFINE* TEST_DEFINE* TEST_DEFINE* TEST_DEFINE* TEST_DEFINE* TEST_DEFINE;
    if (test2 < test3)
    {
        std::cout << test2 << std::endl;
    }
    else
    {
        std::cout << test3 << std::endl;
    }

    const char* processName = "GuiltyGearXrd.exe";
    //const char* processName = "TestLoad.exe";
    int pid = findProcess(processName);

    if (pid)
    {
        std::cout << "found pid: " << pid << std::endl;
    }
    else
    {
        std::cout << "failed to find pid" << std::endl;
        return EXIT_FAILURE;
    }

    injectDll(pid);

    return EXIT_SUCCESS;
}
