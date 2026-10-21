#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <tlhelp32.h>
#include <cstdio>
#include <cstdint>
#include <vector>
#include <string>

using u8 = uint8_t;

static DWORD findProcess(const wchar_t* name) {
    HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snap == INVALID_HANDLE_VALUE) return 0;
    PROCESSENTRY32W pe = { sizeof(pe) };
    DWORD pid = 0;
    if (Process32FirstW(snap, &pe)) {
        do {
            if (_wcsicmp(pe.szExeFile, name) == 0) { pid = pe.th32ProcessID; break; }
        } while (Process32NextW(snap, &pe));
    }
    CloseHandle(snap);
    return pid;
}

static bool enableDebugPriv() {
    HANDLE tok;
    if (!OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &tok)) return false;
    LUID luid;
    if (!LookupPrivilegeValueA(nullptr, "SeDebugPrivilege", &luid)) { CloseHandle(tok); return false; }
    TOKEN_PRIVILEGES tp = {};
    tp.PrivilegeCount = 1;
    tp.Privileges[0].Luid = luid;
    tp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
    bool ok = AdjustTokenPrivileges(tok, FALSE, &tp, sizeof(tp), nullptr, nullptr);
    CloseHandle(tok);
    return ok;
}

static u8 g_shellcode[] = {
    0x48, 0x83, 0xEC, 0x28,
    0x48, 0xB9, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0xBA, 0x01, 0x00, 0x00, 0x00,
    0x4D, 0x31, 0xC0,
    0x48, 0xB8, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0xFF, 0xD0,
    0x48, 0x83, 0xC4, 0x28,
    0xC3
};
static constexpr int SC_BASE_OFF  = 6;
static constexpr int SC_ENTRY_OFF = 23;

static bool manualMap(HANDLE hProc, const char* dllPath) {
    HANDLE hFile = CreateFileA(dllPath, GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, 0, nullptr);
    if (hFile == INVALID_HANDLE_VALUE) { printf("[!] Can't open DLL: %s\n", dllPath); return false; }
    DWORD fileSize = GetFileSize(hFile, nullptr);
    std::vector<u8> raw(fileSize);
    DWORD read;
    ReadFile(hFile, raw.data(), fileSize, &read, nullptr);
    CloseHandle(hFile);

    auto dos = (PIMAGE_DOS_HEADER)raw.data();
    if (dos->e_magic != IMAGE_DOS_SIGNATURE) { printf("[!] Invalid DOS header\n"); return false; }
    auto nt = (PIMAGE_NT_HEADERS)(raw.data() + dos->e_lfanew);
    if (nt->Signature != IMAGE_NT_SIGNATURE) { printf("[!] Invalid NT header\n"); return false; }
    if (nt->FileHeader.Machine != IMAGE_FILE_MACHINE_AMD64) { printf("[!] Not x64\n"); return false; }

    auto imageSize  = nt->OptionalHeader.SizeOfImage;
    auto headerSize = nt->OptionalHeader.SizeOfHeaders;

    auto remoteBase = (u8*)VirtualAllocEx(hProc, (void*)nt->OptionalHeader.ImageBase,
        imageSize, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
    if (!remoteBase) {
        remoteBase = (u8*)VirtualAllocEx(hProc, nullptr, imageSize, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
    }
    if (!remoteBase) { printf("[!] VirtualAllocEx failed: %lu\n", GetLastError()); return false; }
    printf("[+] Allocated %u bytes at %p\n", imageSize, remoteBase);

    std::vector<u8> image(imageSize, 0);
    memcpy(image.data(), raw.data(), headerSize);

    auto sections = IMAGE_FIRST_SECTION(nt);
    for (int i = 0; i < nt->FileHeader.NumberOfSections; i++) {
        if (sections[i].SizeOfRawData > 0) {
            memcpy(image.data() + sections[i].VirtualAddress,
                   raw.data() + sections[i].PointerToRawData,
                   sections[i].SizeOfRawData);
        }
    }

    auto delta = (intptr_t)(remoteBase - nt->OptionalHeader.ImageBase);
    if (delta != 0) {
        auto relocDir = &nt->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC];
        if (relocDir->Size > 0) {
            auto reloc = (PIMAGE_BASE_RELOCATION)(image.data() + relocDir->VirtualAddress);
            while (reloc->VirtualAddress && reloc->SizeOfBlock) {
                auto count   = (reloc->SizeOfBlock - sizeof(IMAGE_BASE_RELOCATION)) / sizeof(WORD);
                auto entries = (WORD*)((u8*)reloc + sizeof(IMAGE_BASE_RELOCATION));
                for (DWORD j = 0; j < count; j++) {
                    auto type   = entries[j] >> 12;
                    auto offset = entries[j] & 0xFFF;
                    if (type == IMAGE_REL_BASED_DIR64) {
                        auto ptr = (uint64_t*)(image.data() + reloc->VirtualAddress + offset);
                        *ptr += delta;
                    } else if (type == IMAGE_REL_BASED_HIGHLOW) {
                        auto ptr = (uint32_t*)(image.data() + reloc->VirtualAddress + offset);
                        *ptr += (uint32_t)delta;
                    }
                }
                reloc = (PIMAGE_BASE_RELOCATION)((u8*)reloc + reloc->SizeOfBlock);
            }
        }
    }

    auto importDir = &nt->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT];
    if (importDir->Size > 0) {
        auto imp = (PIMAGE_IMPORT_DESCRIPTOR)(image.data() + importDir->VirtualAddress);
        while (imp->Name) {
            auto modName = (const char*)(image.data() + imp->Name);
            HMODULE hMod = LoadLibraryA(modName);
            if (!hMod) { printf("[!] Can't load import module: %s\n", modName); imp++; continue; }

            auto thunkOrig = (PIMAGE_THUNK_DATA)(image.data() + (imp->OriginalFirstThunk ? imp->OriginalFirstThunk : imp->FirstThunk));
            auto thunkIAT  = (PIMAGE_THUNK_DATA)(image.data() + imp->FirstThunk);

            while (thunkOrig->u1.AddressOfData) {
                FARPROC func = nullptr;
                if (IMAGE_SNAP_BY_ORDINAL64(thunkOrig->u1.Ordinal)) {
                    func = GetProcAddress(hMod, (LPCSTR)(thunkOrig->u1.Ordinal & 0xFFFF));
                } else {
                    auto name = (PIMAGE_IMPORT_BY_NAME)(image.data() + thunkOrig->u1.AddressOfData);
                    func = GetProcAddress(hMod, name->Name);
                }
                thunkIAT->u1.Function = (ULONGLONG)func;
                thunkOrig++;
                thunkIAT++;
            }
            imp++;
        }
    }

    memset(image.data(), 0, headerSize);

    SIZE_T written;
    if (!WriteProcessMemory(hProc, remoteBase, image.data(), imageSize, &written)) {
        printf("[!] WriteProcessMemory failed: %lu\n", GetLastError());
        VirtualFreeEx(hProc, remoteBase, 0, MEM_RELEASE);
        return false;
    }
    printf("[+] Wrote %zu bytes\n", written);

    auto entryRVA   = nt->OptionalHeader.AddressOfEntryPoint;
    auto entrypoint = (uint64_t)(remoteBase + entryRVA);
    auto base64     = (uint64_t)remoteBase;

    memcpy(g_shellcode + SC_BASE_OFF,  &base64,     8);
    memcpy(g_shellcode + SC_ENTRY_OFF, &entrypoint, 8);

    auto scRemote = VirtualAllocEx(hProc, nullptr, sizeof(g_shellcode), MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
    if (!scRemote) { printf("[!] Shellcode alloc failed\n"); return false; }
    WriteProcessMemory(hProc, scRemote, g_shellcode, sizeof(g_shellcode), nullptr);

    printf("[+] Executing DllMain at %p\n", (void*)entrypoint);

    auto hThread = CreateRemoteThread(hProc, nullptr, 0, (LPTHREAD_START_ROUTINE)scRemote, nullptr, 0, nullptr);
    if (!hThread) { printf("[!] CreateRemoteThread failed: %lu\n", GetLastError()); return false; }

    WaitForSingleObject(hThread, 10000);
    CloseHandle(hThread);
    VirtualFreeEx(hProc, scRemote, 0, MEM_RELEASE);

    printf("[+] DLL mapped and running\n");
    return true;
}

static std::string getExeDir() {
    char buf[MAX_PATH];
    GetModuleFileNameA(nullptr, buf, MAX_PATH);
    std::string s(buf);
    auto pos = s.find_last_of("\\/");
    return pos != std::string::npos ? s.substr(0, pos + 1) : "";
}

static bool isElevated() {
    BOOL elevated = FALSE;
    HANDLE tok = nullptr;
    if (OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &tok)) {
        TOKEN_ELEVATION elev = {};
        DWORD sz = sizeof(elev);
        if (GetTokenInformation(tok, TokenElevation, &elev, sizeof(elev), &sz))
            elevated = elev.TokenIsElevated;
        CloseHandle(tok);
    }
    return elevated != 0;
}

int main() {
    SetConsoleTitleA("Enhanced - GTA V");
    printf("=================================\n");
    printf("  Enhanced | GTA V Mod Menu\n");
    printf("  DX12 | BattlEye Off\n");
    printf("  INSERT = open menu\n");
    printf("  END    = unload\n");
    printf("=================================\n\n");

    if (!isElevated()) {
        printf("[!] NOT running as Administrator!\n");
        printf("    Right-click -> Run as administrator.\n");
        getchar();
        return 1;
    }
    printf("[+] Running elevated\n");

    if (!enableDebugPriv())
        printf("[!] SeDebugPrivilege failed (may still work)\n");
    else
        printf("[+] SeDebugPrivilege enabled\n");

    printf("[*] Waiting for GTA5.exe...\n");
    DWORD pid = 0;
    while (!pid) {
        pid = findProcess(L"GTA5.exe");
        if (!pid) Sleep(500);
    }
    printf("[+] Found GTA5.exe (PID %lu)\n", pid);

    const DWORD accessFlags[] = {
        PROCESS_ALL_ACCESS,
        PROCESS_VM_OPERATION | PROCESS_VM_READ | PROCESS_VM_WRITE |
        PROCESS_CREATE_THREAD | PROCESS_QUERY_INFORMATION
    };
    HANDLE hProc = nullptr;
    bool verified = false;
    printf("[*] Acquiring process handle...\n");
    for (int attempt = 0; attempt < 30 && !verified; attempt++) {
        for (auto flags : accessFlags) {
            HANDLE h = OpenProcess(flags, FALSE, pid);
            if (!h) continue;
            auto test = VirtualAllocEx(h, nullptr, 0x1000, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
            if (test) {
                VirtualFreeEx(h, test, 0, MEM_RELEASE);
                hProc = h;
                verified = true;
                printf("[+] Handle verified (attempt %d, flags 0x%lX)\n", attempt + 1, flags);
                break;
            }
            CloseHandle(h);
        }
        if (!verified) {
            if (attempt == 0)
                printf("[*] Handle neutered, retrying (up to 30s)...\n");
            Sleep(1000);
        }
    }
    if (!hProc) {
        printf("[!] Could not get a working process handle.\n");
        printf("    Make sure BattlEye is OFF and you Run as Admin.\n");
        getchar();
        return 1;
    }

    auto dir = getExeDir();
    auto dllPath = dir + "enhanced.dll";

    DWORD attr = GetFileAttributesA(dllPath.c_str());
    if (attr == INVALID_FILE_ATTRIBUTES) {
        printf("[!] enhanced.dll not found next to injector.\n");
        CloseHandle(hProc);
        getchar();
        return 1;
    }

    printf("[*] Injecting enhanced.dll (manual map)...\n");

    if (!manualMap(hProc, dllPath.c_str())) {
        printf("[!] Injection failed.\n");
        CloseHandle(hProc);
        getchar();
        return 1;
    }

    printf("[+] Done. Menu is loaded in GTA V.\n");
    printf("[*] Press any key to close this window.\n");
    CloseHandle(hProc);
    getchar();
    return 0;
}
