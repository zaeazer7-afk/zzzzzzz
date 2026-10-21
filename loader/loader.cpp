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

// Shellcode that runs in the target — resolves imports + calls DllMain
// We build a small data block + shellcode and execute via thread hijack
#pragma pack(push, 1)
struct ShellcodeData
{
    uint64_t    ImageBase;
    uint64_t    NtHeaders;        // offset from ImageBase to NT headers
    uint64_t    pLoadLibraryA;
    uint64_t    pGetProcAddress;
    uint64_t    Reason;           // DLL_PROCESS_ATTACH
    uint64_t    OrigRip;          // to resume after shellcode
    uint64_t    OrigRsp;
};
#pragma pack(pop)

// x64 shellcode — resolve imports, call DllMain, restore execution
// This is position-independent
static const uint8_t g_Shellcode[] = {
    // save registers
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
    0x48, 0x83, 0xEC, 0x28,                         // sub rsp, 0x28 (shadow space)

    // lea rbx, [rip - offset] to find ShellcodeData placed right before this code
    // We'll use a different approach: data is at a known offset before shellcode
    // rcx = address of ShellcodeData (we set RCX before hijack)

    // Load ShellcodeData fields
    0x48, 0x8B, 0x01,                               // mov rax, [rcx]          ; ImageBase
    0x48, 0x89, 0xC6,                               // mov rsi, rax            ; rsi = ImageBase
    0x48, 0x8B, 0x59, 0x08,                         // mov rbx, [rcx+8]        ; NT headers offset
    0x48, 0x01, 0xF3,                               // add rbx, rsi            ; rbx = &NT_HEADERS
    0x4C, 0x8B, 0x61, 0x10,                         // mov r12, [rcx+0x10]     ; pLoadLibraryA
    0x4C, 0x8B, 0x69, 0x18,                         // mov r13, [rcx+0x18]     ; pGetProcAddress
    0x48, 0x89, 0xCD,                               // mov rbp, rcx            ; save data ptr

    // --- Resolve imports ---
    // Import directory RVA at NT+0x90 (OptionalHeader offset 0x78 + 0x18 for x64)
    0x8B, 0x83, 0x90, 0x00, 0x00, 0x00,            // mov eax, [rbx+0x90]     ; import dir RVA
    0x85, 0xC0,                                      // test eax, eax
    0x0F, 0x84, 0x6A, 0x00, 0x00, 0x00,            // jz skip_imports

    0x48, 0x01, 0xF0,                               // add rax, rsi            ; rax = import descriptor
    0x49, 0x89, 0xC6,                               // mov r14, rax

    // import_loop:
    0x41, 0x8B, 0x46, 0x0C,                         // mov eax, [r14+0x0C]     ; Name RVA
    0x85, 0xC0,                                      // test eax, eax
    0x74, 0x55,                                      // jz skip_imports

    // LoadLibraryA(ImageBase + NameRVA)
    0x48, 0x01, 0xF0,                               // add rax, rsi
    0x48, 0x89, 0xC1,                               // mov rcx, rax
    0x41, 0xFF, 0xD4,                               // call r12                ; LoadLibraryA
    0x49, 0x89, 0xC7,                               // mov r15, rax            ; hModule

    // Walk thunk
    0x41, 0x8B, 0x06,                               // mov eax, [r14]          ; OriginalFirstThunk
    0x85, 0xC0,                                      // test eax, eax
    0x75, 0x04,                                      // jnz has_oft
    0x41, 0x8B, 0x46, 0x10,                         // mov eax, [r14+0x10]     ; FirstThunk
    // has_oft:
    0x48, 0x01, 0xF0,                               // add rax, rsi
    0x48, 0x89, 0xC3,                               // mov rbx, rax            ; rbx = pOFT
    0x41, 0x8B, 0x46, 0x10,                         // mov eax, [r14+0x10]     ; FirstThunk RVA
    0x48, 0x01, 0xF0,                               // add rax, rsi
    0x48, 0x89, 0xC7,                               // mov rdi, rax            ; rdi = pFT

    // thunk_loop:
    0x48, 0x8B, 0x03,                               // mov rax, [rbx]
    0x48, 0x85, 0xC0,                               // test rax, rax
    0x74, 0x1B,                                      // jz next_descriptor

    // Check ordinal flag (bit 63)
    0x48, 0x0F, 0xBA, 0xE0, 0x3F,                  // bt rax, 63
    0x73, 0x05,                                      // jnc by_name
    0x48, 0x25, 0xFF, 0xFF, 0x00, 0x00,            // and rax, 0xFFFF         ; ordinal
    0xEB, 0x04,                                      // jmp do_resolve
    // by_name:
    0x48, 0x01, 0xF0,                               // add rax, rsi            ; &IMPORT_BY_NAME
    0x48, 0x83, 0xC0, 0x02,                         // add rax, 2              ; skip Hint
    // do_resolve:
    0x4C, 0x89, 0xF9,                               // mov rcx, r15            ; hModule
    0x48, 0x89, 0xC2,                               // mov rdx, rax            ; name/ordinal
    0x41, 0xFF, 0xD5,                               // call r13                ; GetProcAddress
    0x48, 0x89, 0x07,                               // mov [rdi], rax          ; store in IAT

    0x48, 0x83, 0xC3, 0x08,                         // add rbx, 8
    0x48, 0x83, 0xC7, 0x08,                         // add rdi, 8
    0xEB, 0xD3,                                      // jmp thunk_loop

    // next_descriptor:
    0x49, 0x83, 0xC6, 0x14,                         // add r14, 0x14 (sizeof IMAGE_IMPORT_DESCRIPTOR)
    0xEB, 0x9F,                                      // jmp import_loop

    // skip_imports:
    // --- Call DllMain(ImageBase, DLL_PROCESS_ATTACH, 0) ---
    // EntryPoint RVA at NT+0x28
    0x48, 0x89, 0xEB,                               // mov rbx, rbp            ; data ptr
    0x48, 0x8B, 0x4B, 0x08,                         // mov rcx, [rbx+8]        ; NT offset
    0x48, 0x03, 0x0B,                               // add rcx, [rbx]          ; + ImageBase = &NT
    0x8B, 0x41, 0x28,                               // mov eax, [rcx+0x28]     ; AddressOfEntryPoint
    0x85, 0xC0,                                      // test eax, eax
    0x74, 0x10,                                      // jz no_entry

    0x48, 0x03, 0x03,                               // add rax, [rbx]          ; EntryPoint VA
    0x48, 0x8B, 0x0B,                               // mov rcx, [rbx]          ; hinstDLL = ImageBase
    0x48, 0x8B, 0x53, 0x20,                         // mov rdx, [rbx+0x20]     ; fdwReason = 1
    0x4D, 0x31, 0xC0,                               // xor r8, r8              ; lpReserved = 0
    0xFF, 0xD0,                                      // call rax                ; DllMain

    // no_entry:
    // Restore registers and jump to original RIP
    0x48, 0x83, 0xC4, 0x28,                         // add rsp, 0x28
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
    // 8 bytes follow: the original RIP to resume
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
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

    // Allocate shellcode + data in target
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

    // Build data block
    ShellcodeData data{};
    data.ImageBase      = (uint64_t)remoteBase;
    data.NtHeaders      = (uint64_t)dos->e_lfanew;
    data.pLoadLibraryA  = (uint64_t)GetProcAddress( GetModuleHandleA("kernel32.dll"), "LoadLibraryA" );
    data.pGetProcAddress = (uint64_t)GetProcAddress( GetModuleHandleA("kernel32.dll"), "GetProcAddress" );
    data.Reason         = DLL_PROCESS_ATTACH;
    data.OrigRip        = ctx.Rip;
    data.OrigRsp        = ctx.Rsp;

    // Build shellcode with patched resume address
    std::vector<uint8_t> sc( g_Shellcode, g_Shellcode + sizeof(g_Shellcode) );
    // Patch the last 8 bytes (jmp target) with original RIP
    memcpy( sc.data() + sc.size() - 8, &ctx.Rip, 8 );

    // Write data + shellcode
    SIZE_T written = 0;
    uint8_t* dataAddr = (uint8_t*)scRemote;
    uint8_t* codeAddr = dataAddr + sizeof( ShellcodeData );

    Syscall::pNtWriteVirtualMemory( hProc, dataAddr, &data, sizeof(data), &written );
    Syscall::pNtWriteVirtualMemory( hProc, codeAddr, sc.data(), sc.size(), &written );

    // Set executable
    ULONG oldProt = 0;
    void* protAddr = scRemote;
    SIZE_T protSize = scSize;
    Syscall::pNtProtectVirtualMemory( hProc, &protAddr, &protSize, PAGE_EXECUTE_READ, &oldProt );

    // Hijack thread — RCX = data ptr, RIP = shellcode
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
    const wchar_t* targets[] = { L"FiveM_b3095_GTAProcess.exe", L"FiveM_b2944_GTAProcess.exe",
                                  L"FiveM_b2802_GTAProcess.exe", L"FiveM_b2699_GTAProcess.exe",
                                  L"FiveM_GTAProcess.exe", L"GTA5.exe" };

    DWORD pid = 0;
    for ( int attempt = 0; attempt < 300 && !pid; attempt++ )
    {
        for ( auto t : targets )
        {
            pid = FindProcess( t );
            if ( pid ) break;
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
    printf( "[+] Allocated at 0x%p (%zu KB)\n", remoteBase, nt->OptionalHeader.SizeOfImage / 1024 );

    printf( "[*] Mapping sections...\n" );
    MapSections( hProc, dll.data(), remoteBase );

    printf( "[*] Fixing relocations...\n" );
    FixRelocations( hProc, dll.data(), remoteBase );

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
