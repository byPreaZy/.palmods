#include <windows.h>
#include <tlhelp32.h>
#include <string.h>
#include <cstdio>
#include "logger.hpp"

static DWORD FindProcessId(const wchar_t* name) {
    DWORD pid = 0;
    HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snap != INVALID_HANDLE_VALUE) {
        PROCESSENTRY32W pe; pe.dwSize = sizeof(pe);
        if (Process32FirstW(snap, &pe)) {
            do {
                if (_wcsicmp(pe.szExeFile, name) == 0) {
                    pid = pe.th32ProcessID;
                    break;
                }
            } while (Process32NextW(snap, &pe));
        }
        CloseHandle(snap);
    }
    return pid;
}

int wmain(int argc, wchar_t* argv[]) {
    LogInit(nullptr);
    Log("=== PalTrainerInjector v1.0 starting ===");
    const wchar_t* dllPath = (argc > 1) ? argv[1] : L"PalTrainerCore.dll";
    if (argc > 1) dllPath = argv[1];
    DWORD pid = FindProcessId(L"Palworld-Win64-Shipping.exe");
    if (!pid) {
        Log("Injector: Palworld not found");
        MessageBoxW(NULL, L"Palworld process not found.", L"PalTrainerUltra Injector", MB_OK | MB_ICONERROR);
        LogClose();
        return 1;
    }
    Log("Injector: Palworld found pid=%lu", pid);
    HANDLE hProc = OpenProcess(PROCESS_CREATE_THREAD | PROCESS_VM_OPERATION | PROCESS_VM_WRITE | PROCESS_QUERY_LIMITED_INFORMATION, FALSE, pid);
    if (!hProc) {
        Log("Injector: OpenProcess failed");
        MessageBoxW(NULL, L"OpenProcess failed.", L"PalTrainerUltra Injector", MB_OK | MB_ICONERROR);
        LogClose();
        return 1;
    }
    SIZE_T len = (wcslen(dllPath) + 1) * sizeof(wchar_t);
    LPVOID remote = VirtualAllocEx(hProc, NULL, len, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
    if (!remote || !WriteProcessMemory(hProc, remote, dllPath, len, NULL)) {
        Log("Injector: Remote memory write failed");
        MessageBoxW(NULL, L"Remote memory write failed.", L"PalTrainerUltra Injector", MB_OK | MB_ICONERROR);
        LogClose();
        return 1;
    }
    Log("Injector: DLL path written to remote memory");
    HMODULE hKernel = GetModuleHandleW(L"kernel32.dll");
    LPTHREAD_START_ROUTINE load = (LPTHREAD_START_ROUTINE)GetProcAddress(hKernel, "LoadLibraryW");
    HANDLE hThread = CreateRemoteThread(hProc, NULL, 0, load, remote, 0, NULL);
    if (!hThread) {
        Log("Injector: CreateRemoteThread failed");
        MessageBoxW(NULL, L"CreateRemoteThread failed.", L"PalTrainerUltra Injector", MB_OK | MB_ICONERROR);
        LogClose();
        return 1;
    }
    WaitForSingleObject(hThread, INFINITE);
    CloseHandle(hThread);
    VirtualFreeEx(hProc, remote, 0, MEM_RELEASE);
    CloseHandle(hProc);
    Log("Injector: injection complete");
    MessageBoxW(NULL, L"Injection complete.", L"PalTrainerUltra Injector", MB_OK | MB_ICONINFORMATION);
    LogClose();
    return 0;
}
