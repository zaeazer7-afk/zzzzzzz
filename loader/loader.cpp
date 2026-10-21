#include <Windows.h>
#include <TlHelp32.h>
#include <winternl.h>
#include <winhttp.h>
#include <cstdio>
#include <cstdint>
#include <cstring>
#include <vector>
#include <string>
#include <fstream>
#include <sstream>

#pragma comment(lib, "winhttp.lib")

#include "payload.hpp"

// ---------------------------------------------------------
// Syscall stubs — resolve from ntdll on disk to bypass hooks
// ---------------------------------------------------------

namespace Syscall
{
    struct StubInfo
    {
        uint32_t Number;
        void*    Trampoline;
    };

    static uint32_t ResolveSyscallNumber( const char* fnName )
    {
        HMODULE ntdll = GetModuleHandleA( "ntdll.dll" );
        if ( !ntdll ) return 0;
        auto* fn = reinterpret_cast<uint8_t*>( GetProcAddress( ntdll, fnName ) );
        if ( !fn ) return 0;

        // mov r10, rcx; mov eax, <syscall#>
        if ( fn[0] == 0x4C && fn[1] == 0x8B && fn[2] == 0xD1 &&
             fn[3] == 0xB8 )
        {
            return *reinterpret_cast<uint32_t*>( fn + 4 );
        }

        // hooked — scan nearby for the pattern
        for ( int delta = -32; delta <= 32; delta += 16 )
        {
            if ( delta == 0 ) continue;
            auto* nearby = fn + delta;
            if ( nearby[0] == 0x4C && nearby[1] == 0x8B && nearby[2] == 0xD1 &&
                 nearby[3] == 0xB8 )
            {
                uint32_t num = *reinterpret_cast<uint32_t*>( nearby + 4 );
                int offset = delta / 16;
                return static_cast<uint32_t>( static_cast<int>( num ) - offset );
            }
        }
        return 0;
    }

    static void* BuildTrampoline( uint32_t number )
    {
        // 20-byte stub: mov r10,rcx / mov eax,N / syscall / ret
        static uint8_t stubs[16][20];
        static int idx = 0;
        if ( idx >= 16 ) return nullptr;

        uint8_t* s = stubs[idx++];
        DWORD old;
        VirtualProtect( s, 20, PAGE_EXECUTE_READWRITE, &old );

        s[0] = 0x4C; s[1] = 0x8B; s[2] = 0xD1;       // mov r10, rcx
        s[3] = 0xB8;                                     // mov eax, imm32
        memcpy( s + 4, &number, 4 );
        s[8] = 0x0F; s[9] = 0x05;                       // syscall
        s[10] = 0xC3;                                    // ret

        return s;
    }

    static StubInfo Resolve( const char* name )
    {
        uint32_t num = ResolveSyscallNumber( name );
        void* tramp = num ? BuildTrampoline( num ) : nullptr;
        return { num, tramp };
    }

    // Typed wrappers
    using fn_NtAllocateVirtualMemory  = NTSTATUS(NTAPI*)( HANDLE, PVOID*, ULONG_PTR, PSIZE_T, ULONG, ULONG );
    using fn_NtWriteVirtualMemory     = NTSTATUS(NTAPI*)( HANDLE, PVOID, PVOID, SIZE_T, PSIZE_T );
    using fn_NtProtectVirtualMemory   = NTSTATUS(NTAPI*)( HANDLE, PVOID*, PSIZE_T, ULONG, PULONG );
    using fn_NtFreeVirtualMemory      = NTSTATUS(NTAPI*)( HANDLE, PVOID*, PSIZE_T, ULONG );

    fn_NtAllocateVirtualMemory  pNtAllocateVirtualMemory  = nullptr;
    fn_NtWriteVirtualMemory     pNtWriteVirtualMemory     = nullptr;
    fn_NtProtectVirtualMemory   pNtProtectVirtualMemory   = nullptr;
    fn_NtFreeVirtualMemory      pNtFreeVirtualMemory      = nullptr;

    bool Init()
    {
        auto alloc   = Resolve( "NtAllocateVirtualMemory" );
        auto write   = Resolve( "NtWriteVirtualMemory" );
        auto protect = Resolve( "NtProtectVirtualMemory" );
        auto free_   = Resolve( "NtFreeVirtualMemory" );

        if ( !alloc.Trampoline || !write.Trampoline || !protect.Trampoline || !free_.Trampoline )
            return false;

        pNtAllocateVirtualMemory  = (fn_NtAllocateVirtualMemory) alloc.Trampoline;
        pNtWriteVirtualMemory     = (fn_NtWriteVirtualMemory)    write.Trampoline;
        pNtProtectVirtualMemory   = (fn_NtProtectVirtualMemory)  protect.Trampoline;
        pNtFreeVirtualMemory      = (fn_NtFreeVirtualMemory)     free_.Trampoline;
        return true;
    }
}

// ---------------------------------------------------------
// Process utilities
// ---------------------------------------------------------

static DWORD FindProcess( const wchar_t* name )
{
    HANDLE snap = CreateToolhelp32Snapshot( TH32CS_SNAPPROCESS, 0 );
    if ( snap == INVALID_HANDLE_VALUE ) return 0;

    PROCESSENTRY32W pe{};
    pe.dwSize = sizeof( pe );

    DWORD pid = 0;
    if ( Process32FirstW( snap, &pe ) )
    {
        do {
            if ( _wcsicmp( pe.szExeFile, name ) == 0 )
            {
                pid = pe.th32ProcessID;
                break;
            }
        } while ( Process32NextW( snap, &pe ) );
    }
    CloseHandle( snap );
    return pid;
}

static DWORD FindThread( DWORD pid )
{
    HANDLE snap = CreateToolhelp32Snapshot( TH32CS_SNAPTHREAD, 0 );
    if ( snap == INVALID_HANDLE_VALUE ) return 0;

    THREADENTRY32 te{};
    te.dwSize = sizeof( te );

    DWORD tid = 0;
    if ( Thread32First( snap, &te ) )
    {
        do {
            if ( te.th32OwnerProcessID == pid )
            {
                tid = te.th32ThreadID;
                break;
            }
        } while ( Thread32Next( snap, &te ) );
    }
    CloseHandle( snap );
    return tid;
}

// ---------------------------------------------------------
// Decrypt embedded payload
// ---------------------------------------------------------

static std::vector<uint8_t> DecryptPayload()
{
    std::vector<uint8_t> out( g_PayloadSize );
    for ( uint32_t i = 0; i < g_PayloadSize; i++ )
        out[i] = g_PayloadData[i] ^ g_PayloadKey[i % 32];
    return out;
}

// ---------------------------------------------------------
// Manual mapper
// ---------------------------------------------------------

struct MappedImage
{
    void*    RemoteBase;
    size_t   ImageSize;
};

static bool MapSections( HANDLE hProc, const uint8_t* raw, void* remoteBase )
{
    auto* dos = reinterpret_cast<const IMAGE_DOS_HEADER*>( raw );
    auto* nt  = reinterpret_cast<const IMAGE_NT_HEADERS*>( raw + dos->e_lfanew );
    auto* sec = IMAGE_FIRST_SECTION( nt );

    // Write headers
    SIZE_T written = 0;
    Syscall::pNtWriteVirtualMemory( hProc, remoteBase, (PVOID)raw,
        nt->OptionalHeader.SizeOfHeaders, &written );

    for ( WORD i = 0; i < nt->FileHeader.NumberOfSections; i++ )
    {
        if ( sec[i].SizeOfRawData == 0 ) continue;
        void* dest = (uint8_t*)remoteBase + sec[i].VirtualAddress;
        Syscall::pNtWriteVirtualMemory( hProc, dest,
            (PVOID)(raw + sec[i].PointerToRawData),
            sec[i].SizeOfRawData, &written );
    }
    return true;
}

static bool FixRelocations( HANDLE hProc, const uint8_t* raw, void* remoteBase )
{
    auto* dos = reinterpret_cast<const IMAGE_DOS_HEADER*>( raw );
    auto* nt  = reinterpret_cast<const IMAGE_NT_HEADERS*>( raw + dos->e_lfanew );

    auto& relocDir = nt->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC];
    if ( !relocDir.Size ) return true;

    intptr_t delta = (intptr_t)remoteBase - (intptr_t)nt->OptionalHeader.ImageBase;
    if ( delta == 0 ) return true;

    // Read the mapped image back for patching
    std::vector<uint8_t> img( nt->OptionalHeader.SizeOfImage );
    SIZE_T rd = 0;
    ReadProcessMemory( hProc, remoteBase, img.data(), img.size(), &rd );

    auto* block = reinterpret_cast<IMAGE_BASE_RELOCATION*>( img.data() + relocDir.VirtualAddress );
    auto* end   = reinterpret_cast<IMAGE_BASE_RELOCATION*>( img.data() + relocDir.VirtualAddress + relocDir.Size );

    while ( block < end && block->SizeOfBlock > 0 )
    {
        uint32_t count = ( block->SizeOfBlock - sizeof( IMAGE_BASE_RELOCATION ) ) / sizeof( uint16_t );
        auto* entries = reinterpret_cast<uint16_t*>( block + 1 );

        for ( uint32_t i = 0; i < count; i++ )
        {
            uint16_t type   = entries[i] >> 12;
            uint16_t offset = entries[i] & 0xFFF;

            if ( type == IMAGE_REL_BASED_DIR64 )
            {
                uint64_t* patch = reinterpret_cast<uint64_t*>( img.data() + block->VirtualAddress + offset );
                *patch += delta;
            }
            else if ( type == IMAGE_REL_BASED_HIGHLOW )
            {
                uint32_t* patch = reinterpret_cast<uint32_t*>( img.data() + block->VirtualAddress + offset );
                *patch += static_cast<uint32_t>( delta );
            }
        }

        block = reinterpret_cast<IMAGE_BASE_RELOCATION*>( (uint8_t*)block + block->SizeOfBlock );
    }

    // Write patched image back
    SIZE_T written = 0;
    Syscall::pNtWriteVirtualMemory( hProc, remoteBase, img.data(), img.size(), &written );
    return true;
}

// RVA-to-file-offset converter for raw PE on disk
static uint32_t RvaToOffset( const uint8_t* raw, uint32_t rva )
{
    auto* dos = reinterpret_cast<const IMAGE_DOS_HEADER*>( raw );
    auto* nt  = reinterpret_cast<const IMAGE_NT_HEADERS*>( raw + dos->e_lfanew );
    auto* sec = IMAGE_FIRST_SECTION( nt );
    for ( WORD i = 0; i < nt->FileHeader.NumberOfSections; i++ )
    {
        if ( rva >= sec[i].VirtualAddress &&
             rva < sec[i].VirtualAddress + sec[i].Misc.VirtualSize )
            return sec[i].PointerToRawData + ( rva - sec[i].VirtualAddress );
    }
    return rva;
}

// Resolve imports from our process and write to target's IAT via NtWriteVirtualMemory.
// System DLLs (kernel32, ntdll, ucrtbase, d3d11, etc.) share base addresses across processes.
static bool ResolveImports( HANDLE hProc, const uint8_t* raw, void* remoteBase )
{
    auto* dos = reinterpret_cast<const IMAGE_DOS_HEADER*>( raw );
    auto* nt  = reinterpret_cast<const IMAGE_NT_HEADERS*>( raw + dos->e_lfanew );
    auto& dir = nt->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT];
    if ( !dir.Size || !dir.VirtualAddress ) return true;

    auto* desc = reinterpret_cast<const IMAGE_IMPORT_DESCRIPTOR*>(
        raw + RvaToOffset( raw, dir.VirtualAddress ) );

    while ( desc->Name )
    {
        const char* dllName = reinterpret_cast<const char*>(
            raw + RvaToOffset( raw, desc->Name ) );
        HMODULE hMod = LoadLibraryA( dllName );
        if ( !hMod ) { printf( "[-] Failed to load %s\n", dllName ); return false; }

        uint32_t oftRva = desc->OriginalFirstThunk ? desc->OriginalFirstThunk : desc->FirstThunk;
        auto* pOFT = reinterpret_cast<const IMAGE_THUNK_DATA64*>(
            raw + RvaToOffset( raw, oftRva ) );

        auto* remoteFT = reinterpret_cast<uint64_t*>( (uint8_t*)remoteBase + desc->FirstThunk );

        for ( int idx = 0; pOFT[idx].u1.AddressOfData; idx++ )
        {
            uint64_t resolved = 0;
            if ( IMAGE_SNAP_BY_ORDINAL64( pOFT[idx].u1.Ordinal ) )
            {
                resolved = (uint64_t)GetProcAddress( hMod,
                    MAKEINTRESOURCEA( IMAGE_ORDINAL64( pOFT[idx].u1.Ordinal ) ) );
            }
            else
            {
                auto* ibn = reinterpret_cast<const IMAGE_IMPORT_BY_NAME*>(
                    raw + RvaToOffset( raw, (uint32_t)( pOFT[idx].u1.AddressOfData & 0xFFFFFFFF ) ) );
                resolved = (uint64_t)GetProcAddress( hMod, ibn->Name );
            }
            if ( !resolved ) { printf( "[-] Unresolved import from %s\n", dllName ); return false; }

            SIZE_T written = 0;
            Syscall::pNtWriteVirtualMemory( hProc, &remoteFT[idx], &resolved, sizeof(resolved), &written );
        }
        desc++;
    }
    return true;
}

// Shellcode data — imports already resolved, just call DllMain
#pragma pack(push, 1)
struct ShellcodeData
{
    uint64_t    EntryPoint;       // VA of DllMain (0 if no entry point)
    uint64_t    ImageBase;        // hinstDLL
};
#pragma pack(pop)

// Minimal x64 shellcode — save regs, align stack, call DllMain, restore, resume
static const uint8_t g_Shellcode[] = {
    0x50,                                           // push rax
    0x51,                                           // push rcx
    0x52,                                           // push rdx
    0x53,                                           // push rbx
    0x55,                                           // push rbp
    0x56,                                           // push rsi
    0x57,                                           // push rdi
    0x41, 0x50,                                     // push r8
    0x41, 0x51,                                     // push r9
    0x41, 0x52,                                     // push r10
    0x41, 0x53,                                     // push r11
    0x41, 0x54,                                     // push r12
    0x41, 0x55,                                     // push r13
    0x41, 0x56,                                     // push r14
    0x41, 0x57,                                     // push r15

    0x48, 0x89, 0xE5,                               // mov rbp, rsp
    0x48, 0x83, 0xE4, 0xF0,                         // and rsp, -16
    0x48, 0x83, 0xEC, 0x20,                         // sub rsp, 0x20 (shadow space)

    // rcx = &ShellcodeData (set via thread hijack)
    0x48, 0x8B, 0x01,                               // mov rax, [rcx]       ; EntryPoint
    0x48, 0x85, 0xC0,                               // test rax, rax
    0x74, 0x12,                                     // jz no_entry (+18)

    0x49, 0x89, 0xCC,                               // mov r12, rcx         ; save data ptr
    0x49, 0x8B, 0x4C, 0x24, 0x08,                   // mov rcx, [r12+8]     ; ImageBase = hinstDLL
    0xBA, 0x01, 0x00, 0x00, 0x00,                   // mov edx, 1           ; DLL_PROCESS_ATTACH
    0x4D, 0x31, 0xC0,                               // xor r8, r8           ; lpReserved = 0
    0xFF, 0xD0,                                     // call rax              ; DllMain

    // no_entry:
    0x48, 0x89, 0xEC,                               // mov rsp, rbp

    0x41, 0x5F,                                     // pop r15
    0x41, 0x5E,                                     // pop r14
    0x41, 0x5D,                                     // pop r13
    0x41, 0x5C,                                     // pop r12
    0x41, 0x5B,                                     // pop r11
    0x41, 0x5A,                                     // pop r10
    0x41, 0x59,                                     // pop r9
    0x41, 0x58,                                     // pop r8
    0x5F,                                           // pop rdi
    0x5E,                                           // pop rsi
    0x5D,                                           // pop rbp
    0x5B,                                           // pop rbx
    0x5A,                                           // pop rdx
    0x59,                                           // pop rcx
    0x58,                                           // pop rax

    0xFF, 0x25, 0x00, 0x00, 0x00, 0x00,            // jmp [rip+0]
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 // original RIP
};

static bool InjectViaThreadHijack( HANDLE hProc, DWORD tid, void* remoteBase, const uint8_t* raw )
{
    auto* dos = reinterpret_cast<const IMAGE_DOS_HEADER*>( raw );
    auto* nt  = reinterpret_cast<const IMAGE_NT_HEADERS*>( raw + dos->e_lfanew );

    HANDLE hThread = OpenThread( THREAD_SUSPEND_RESUME | THREAD_GET_CONTEXT | THREAD_SET_CONTEXT, FALSE, tid );
    if ( !hThread ) return false;

    SuspendThread( hThread );

    CONTEXT ctx{};
    ctx.ContextFlags = CONTEXT_FULL;
    GetThreadContext( hThread, &ctx );

    size_t totalSize = sizeof( ShellcodeData ) + sizeof( g_Shellcode );
    void* scRemote = nullptr;
    SIZE_T scSize = totalSize + 64;
    Syscall::pNtAllocateVirtualMemory( hProc, &scRemote, 0, &scSize, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE );

    if ( !scRemote )
    {
        ResumeThread( hThread );
        CloseHandle( hThread );
        return false;
    }

    ShellcodeData data{};
    uint32_t entryRva = nt->OptionalHeader.AddressOfEntryPoint;
    data.EntryPoint = entryRva ? (uint64_t)remoteBase + entryRva : 0;
    data.ImageBase  = (uint64_t)remoteBase;

    std::vector<uint8_t> sc( g_Shellcode, g_Shellcode + sizeof(g_Shellcode) );
    memcpy( sc.data() + sc.size() - 8, &ctx.Rip, 8 );

    SIZE_T written = 0;
    uint8_t* dataAddr = (uint8_t*)scRemote;
    uint8_t* codeAddr = dataAddr + sizeof( ShellcodeData );

    Syscall::pNtWriteVirtualMemory( hProc, dataAddr, &data, sizeof(data), &written );
    Syscall::pNtWriteVirtualMemory( hProc, codeAddr, sc.data(), sc.size(), &written );

    ULONG oldProt = 0;
    void* protAddr = scRemote;
    SIZE_T protSize = scSize;
    Syscall::pNtProtectVirtualMemory( hProc, &protAddr, &protSize, PAGE_EXECUTE_READ, &oldProt );

    ctx.Rcx = (DWORD64)dataAddr;
    ctx.Rip = (DWORD64)codeAddr;
    SetThreadContext( hThread, &ctx );

    ResumeThread( hThread );
    CloseHandle( hThread );
    return true;
}

static bool EraseHeaders( HANDLE hProc, void* remoteBase )
{
    uint8_t zeros[0x1000]{};
    SIZE_T written = 0;

    void* protAddr = remoteBase;
    SIZE_T protSize = 0x1000;
    ULONG oldProt = 0;
    Syscall::pNtProtectVirtualMemory( hProc, &protAddr, &protSize, PAGE_READWRITE, &oldProt );
    Syscall::pNtWriteVirtualMemory( hProc, remoteBase, zeros, 0x1000, &written );
    Syscall::pNtProtectVirtualMemory( hProc, &protAddr, &protSize, oldProt, &oldProt );

    return written > 0;
}

static bool SetSectionProtections( HANDLE hProc, const uint8_t* raw, void* remoteBase )
{
    auto* dos = reinterpret_cast<const IMAGE_DOS_HEADER*>( raw );
    auto* nt  = reinterpret_cast<const IMAGE_NT_HEADERS*>( raw + dos->e_lfanew );
    auto* sec = IMAGE_FIRST_SECTION( nt );

    for ( WORD i = 0; i < nt->FileHeader.NumberOfSections; i++ )
    {
        DWORD prot = PAGE_READONLY;
        bool exec  = ( sec[i].Characteristics & IMAGE_SCN_MEM_EXECUTE ) != 0;
        bool write = ( sec[i].Characteristics & IMAGE_SCN_MEM_WRITE )   != 0;

        if ( exec && write )       prot = PAGE_EXECUTE_READWRITE;
        else if ( exec )           prot = PAGE_EXECUTE_READ;
        else if ( write )          prot = PAGE_READWRITE;

        void*  addr = (uint8_t*)remoteBase + sec[i].VirtualAddress;
        SIZE_T size = sec[i].Misc.VirtualSize;
        if ( size == 0 ) size = sec[i].SizeOfRawData;
        ULONG old = 0;
        Syscall::pNtProtectVirtualMemory( hProc, &addr, &size, prot, &old );
    }
    return true;
}

// ---------------------------------------------------------
// Auth — key validation + HWID
// ---------------------------------------------------------

namespace Auth
{
    static const char* AUTH_HOST = "40.160.149.164";
    static const int   AUTH_PORT = 8888;

    static std::string GetHWID()
    {
        char volName[MAX_PATH], fsName[MAX_PATH];
        DWORD serial = 0, maxLen = 0, flags = 0;
        GetVolumeInformationA( "C:\\", volName, MAX_PATH, &serial, &maxLen, &flags, fsName, MAX_PATH );

        char compName[MAX_COMPUTERNAME_LENGTH + 1];
        DWORD compSize = sizeof( compName );
        GetComputerNameA( compName, &compSize );

        char combined[256];
        snprintf( combined, sizeof(combined), "%08lX-%s", serial, compName );

        unsigned int hash = 0x811C9DC5u;
        for ( int i = 0; combined[i]; i++ )
        {
            hash ^= (unsigned char)combined[i];
            hash *= 0x01000193u;
        }

        char hwid[32];
        snprintf( hwid, sizeof(hwid), "%08X%08lX", hash, serial );
        return hwid;
    }

    static std::string GetKeyPath()
    {
        char appdata[MAX_PATH];
        if ( GetEnvironmentVariableA( "APPDATA", appdata, MAX_PATH ) == 0 )
            return "trident.key";
        return std::string( appdata ) + "\\trident.key";
    }

    static std::string LoadSavedKey()
    {
        std::ifstream f( GetKeyPath() );
        if ( !f.is_open() ) return "";
        std::string key;
        std::getline( f, key );
        return key;
    }

    static void SaveKey( const std::string& key )
    {
        std::ofstream f( GetKeyPath() );
        f << key;
    }

    static bool Validate( const std::string& key, const std::string& hwid )
    {
        HINTERNET hSession = WinHttpOpen( L"Trident/1.0", WINHTTP_ACCESS_TYPE_NO_PROXY, nullptr, nullptr, 0 );
        if ( !hSession ) return false;

        wchar_t wHost[64];
        MultiByteToWideChar( CP_ACP, 0, AUTH_HOST, -1, wHost, 64 );

        HINTERNET hConnect = WinHttpConnect( hSession, wHost, AUTH_PORT, 0 );
        if ( !hConnect ) { WinHttpCloseHandle( hSession ); return false; }

        HINTERNET hRequest = WinHttpOpenRequest( hConnect, L"POST", L"/api/auth",
            nullptr, WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES, 0 );
        if ( !hRequest ) { WinHttpCloseHandle( hConnect ); WinHttpCloseHandle( hSession ); return false; }

        std::string body = "{\"key\":\"" + key + "\",\"hwid\":\"" + hwid + "\"}";
        const wchar_t* headers = L"Content-Type: application/json\r\n";

        BOOL sent = WinHttpSendRequest( hRequest, headers, -1,
            (LPVOID)body.c_str(), (DWORD)body.size(), (DWORD)body.size(), 0 );

        if ( !sent || !WinHttpReceiveResponse( hRequest, nullptr ) )
        {
            WinHttpCloseHandle( hRequest );
            WinHttpCloseHandle( hConnect );
            WinHttpCloseHandle( hSession );
            return false;
        }

        DWORD statusCode = 0, statusSize = sizeof( statusCode );
        WinHttpQueryHeaders( hRequest,
            WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER,
            WINHTTP_HEADER_NAME_BY_INDEX, &statusCode, &statusSize, WINHTTP_NO_HEADER_INDEX );

        WinHttpCloseHandle( hRequest );
        WinHttpCloseHandle( hConnect );
        WinHttpCloseHandle( hSession );

        return statusCode == 200;
    }

    static bool Authenticate()
    {
        std::string hwid = GetHWID();
        std::string key = LoadSavedKey();

        if ( !key.empty() )
        {
            printf( "[*] Saved key found, validating...\n" );
            if ( Validate( key, hwid ) )
            {
                printf( "[+] Authenticated.\n" );
                return true;
            }
            printf( "[-] Saved key invalid or expired.\n" );
        }

        for ( int attempt = 0; attempt < 3; attempt++ )
        {
            printf( "\n    Enter license key: " );
            char buf[128] = {};
            if ( !fgets( buf, sizeof(buf), stdin ) ) return false;

            key = buf;
            while ( !key.empty() && (key.back() == '\n' || key.back() == '\r') )
                key.pop_back();

            if ( key.empty() ) continue;

            printf( "[*] Validating...\n" );
            if ( Validate( key, hwid ) )
            {
                SaveKey( key );
                printf( "[+] Authenticated. Key saved.\n" );
                return true;
            }

            printf( "[-] Invalid key, HWID mismatch, or expired. (%d/3)\n", attempt + 1 );
        }

        return false;
    }
}

// ---------------------------------------------------------
// Main
// ---------------------------------------------------------

int main()
{
    SetConsoleTitleA( "System Service Host" );

    if ( !Auth::Authenticate() )
    {
        printf( "\n[-] Authentication failed.\n" );
        Sleep( 3000 );
        return 1;
    }

    printf( "\n[*] Decrypting payload...\n" );
    auto dll = DecryptPayload();

    auto* dos = reinterpret_cast<IMAGE_DOS_HEADER*>( dll.data() );
    if ( dos->e_magic != IMAGE_DOS_SIGNATURE )
    {
        printf( "[-] Invalid PE\n" );
        return 1;
    }
    auto* nt = reinterpret_cast<IMAGE_NT_HEADERS*>( dll.data() + dos->e_lfanew );
    if ( nt->Signature != IMAGE_NT_SIGNATURE || nt->FileHeader.Machine != IMAGE_FILE_MACHINE_AMD64 )
    {
        printf( "[-] Not a valid x64 PE\n" );
        return 1;
    }

    printf( "[*] Initializing syscalls...\n" );
    if ( !Syscall::Init() )
    {
        printf( "[-] Failed to resolve syscalls\n" );
        return 1;
    }

    printf( "[*] Waiting for FiveM...\n" );
    DWORD pid = 0;
    for ( int attempt = 0; attempt < 300 && !pid; attempt++ )
    {
        HANDLE snap = CreateToolhelp32Snapshot( TH32CS_SNAPPROCESS, 0 );
        if ( snap != INVALID_HANDLE_VALUE )
        {
            PROCESSENTRY32W pe{};
            pe.dwSize = sizeof( pe );
            if ( Process32FirstW( snap, &pe ) )
            {
                do {
                    if ( wcsstr( pe.szExeFile, L"GTAProcess" ) ||
                         _wcsicmp( pe.szExeFile, L"GTA5.exe" ) == 0 )
                    {
                        pid = pe.th32ProcessID;
                        break;
                    }
                } while ( Process32NextW( snap, &pe ) );
            }
            CloseHandle( snap );
        }
        if ( !pid ) Sleep( 1000 );
    }

    if ( !pid )
    {
        printf( "[-] FiveM not found after 5 minutes\n" );
        return 1;
    }
    printf( "[+] Found FiveM PID: %lu\n", pid );

    // Wait a few seconds for the process to settle
    Sleep( 3000 );

    HANDLE hProc = OpenProcess( PROCESS_ALL_ACCESS, FALSE, pid );
    if ( !hProc )
    {
        printf( "[-] Failed to open process (run as admin)\n" );
        return 1;
    }

    // Allocate image in target
    SIZE_T imageSize = nt->OptionalHeader.SizeOfImage;
    void* remoteBase = nullptr;
    NTSTATUS st = Syscall::pNtAllocateVirtualMemory( hProc, &remoteBase, 0, &imageSize,
        MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE );
    if ( st != 0 || !remoteBase )
    {
        printf( "[-] Remote alloc failed: 0x%lX\n", st );
        CloseHandle( hProc );
        return 1;
    }
    printf( "[+] Allocated at 0x%p (%u KB)\n", remoteBase, nt->OptionalHeader.SizeOfImage / 1024 );

    printf( "[*] Mapping sections...\n" );
    MapSections( hProc, dll.data(), remoteBase );

    printf( "[*] Fixing relocations...\n" );
    FixRelocations( hProc, dll.data(), remoteBase );

    printf( "[*] Resolving imports...\n" );
    if ( !ResolveImports( hProc, dll.data(), remoteBase ) )
    {
        printf( "[-] Import resolution failed\n" );
        CloseHandle( hProc );
        return 1;
    }

    printf( "[*] Setting section protections...\n" );
    SetSectionProtections( hProc, dll.data(), remoteBase );

    DWORD tid = FindThread( pid );
    if ( !tid )
    {
        printf( "[-] No thread found\n" );
        CloseHandle( hProc );
        return 1;
    }

    printf( "[*] Hijacking thread %lu...\n", tid );
    if ( !InjectViaThreadHijack( hProc, tid, remoteBase, dll.data() ) )
    {
        printf( "[-] Thread hijack failed\n" );
        CloseHandle( hProc );
        return 1;
    }

    Sleep( 2000 );

    printf( "[*] Erasing PE headers...\n" );
    EraseHeaders( hProc, remoteBase );

    // Clear decrypted DLL from our own memory
    SecureZeroMemory( dll.data(), dll.size() );

    printf( "[+] Done. Press INSERT in-game to open menu.\n" );
    CloseHandle( hProc );

    Sleep( 3000 );
    return 0;
}
