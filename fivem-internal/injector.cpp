// injector.cpp — simple DLL injector for sakura.dll into FiveM
// Compile: cl /EHsc /std:c++20 injector.cpp /link /out:inject.exe
// Usage:   inject.exe
// Finds FiveM process automatically, injects sakura.dll from same directory.

#include <Windows.h>
#include <TlHelp32.h>
#include <iostream>
#include <string>
#include <filesystem>

DWORD FindProcess(const wchar_t* name) {
    HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snap == INVALID_HANDLE_VALUE) return 0;

    PROCESSENTRY32W pe = { sizeof(pe) };
    for (BOOL ok = Process32FirstW(snap, &pe); ok; ok = Process32NextW(snap, &pe)) {
        if (_wcsicmp(pe.szExeFile, name) == 0) {
            CloseHandle(snap);
            return pe.th32ProcessID;
        }
    }
    CloseHandle(snap);
    return 0;
}

bool Inject(DWORD pid, const std::wstring& dllPath) {
    HANDLE proc = OpenProcess(PROCESS_ALL_ACCESS, FALSE, pid);
    if (!proc) {
        std::cerr << "[!] OpenProcess failed: " << GetLastError() << "\n";
        return false;
    }

    size_t pathSize = (dllPath.size() + 1) * sizeof(wchar_t);
    void* mem = VirtualAllocEx(proc, nullptr, pathSize, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
    if (!mem) {
        std::cerr << "[!] VirtualAllocEx failed\n";
        CloseHandle(proc);
        return false;
    }

    WriteProcessMemory(proc, mem, dllPath.c_str(), pathSize, nullptr);

    HMODULE k32 = GetModuleHandleW(L"kernel32.dll");
    auto loadLib = (LPTHREAD_START_ROUTINE)GetProcAddress(k32, "LoadLibraryW");

    HANDLE thread = CreateRemoteThread(proc, nullptr, 0, loadLib, mem, 0, nullptr);
    if (!thread) {
        std::cerr << "[!] CreateRemoteThread failed: " << GetLastError() << "\n";
        VirtualFreeEx(proc, mem, 0, MEM_RELEASE);
        CloseHandle(proc);
        return false;
    }

    WaitForSingleObject(thread, 10000);
    VirtualFreeEx(proc, mem, 0, MEM_RELEASE);
    CloseHandle(thread);
    CloseHandle(proc);
    return true;
}

int main() {
    std::cout << R"(
   ____        __
  / __/__ __  / /__ __ ________
 _\ \/ _ `/ //_/ // / __/ _ `/
/___/\_,_/ /_/\_,_/_/  \_,_/
       FiveM Injector
)" << "\n";

    // get dll path (same folder as injector)
    wchar_t exePath[MAX_PATH];
    GetModuleFileNameW(nullptr, exePath, MAX_PATH);
    std::filesystem::path dllPath = std::filesystem::path(exePath).parent_path() / "sakura.dll";

    if (!std::filesystem::exists(dllPath)) {
        std::cerr << "[!] sakura.dll not found next to injector\n";
        std::cout << "[*] Place sakura.dll in the same folder as inject.exe\n";
        system("pause");
        return 1;
    }

    std::cout << "[*] DLL: " << dllPath.string() << "\n";
    std::cout << "[*] Looking for FiveM...\n";

    // try multiple process names FiveM uses
    const wchar_t* targets[] = { L"FiveM_b3095_GTAProcess.exe",
                                  L"FiveM_b2944_GTAProcess.exe",
                                  L"FiveM_b2802_GTAProcess.exe",
                                  L"FiveM_b2699_GTAProcess.exe",
                                  L"FiveM_b2612_GTAProcess.exe",
                                  L"FiveM_GTAProcess.exe",
                                  L"GTA5.exe" };
    DWORD pid = 0;
    const wchar_t* found = nullptr;
    for (auto t : targets) {
        pid = FindProcess(t);
        if (pid) { found = t; break; }
    }

    if (!pid) {
        std::cerr << "[!] FiveM not found. Launch FiveM first, join a server, then inject.\n";
        system("pause");
        return 1;
    }

    std::wcout << L"[+] Found: " << found << L" (PID " << pid << L")\n";
    std::cout << "[*] Injecting...\n";

    if (Inject(pid, dllPath.wstring())) {
        std::cout << "[+] Injected! Press INSERT in-game to open menu.\n";
    } else {
        std::cerr << "[!] Injection failed. Run as Administrator.\n";
    }

    system("pause");
    return 0;
}
