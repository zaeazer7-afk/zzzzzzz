#pragma once

#include <Windows.h>
#include <cstdint>

namespace Utils
{
    // Dynamically resolve syscall numbers from ntdll at runtime.
    // Avoids static imports that ACs scan for.

    inline uint32_t GetSyscallNumber( const char* funcName )
    {
        HMODULE ntdll = GetModuleHandleA( "ntdll.dll" );
        if ( !ntdll ) return 0;

        auto fn = reinterpret_cast<uint8_t*>( GetProcAddress( ntdll, funcName ) );
        if ( !fn ) return 0;

        // Nt* functions start with: mov r10, rcx (49 89 CA) then mov eax, SSN (B8 XX XX 00 00)
        // or on newer Win11: mov r10, rcx (4C 8B D1) then mov eax, SSN (B8 XX XX 00 00)
        for ( int i = 0; i < 32; i++ )
        {
            if ( fn[i] == 0xB8 )
                return *reinterpret_cast<uint32_t*>( fn + i + 1 );
        }
        return 0;
    }

    // Build a dynamic syscall stub in executable memory.
    // Returns a callable function pointer.
    // Stub: mov r10, rcx / mov eax, SSN / syscall / ret
    inline void* BuildSyscallStub( uint32_t ssn )
    {
        static uint8_t* stubPool = nullptr;
        static size_t   stubOffset = 0;
        constexpr size_t POOL_SIZE = 4096;
        constexpr size_t STUB_SIZE = 16;

        if ( !stubPool )
        {
            stubPool = reinterpret_cast<uint8_t*>(
                VirtualAlloc( nullptr, POOL_SIZE, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE ) );
            if ( !stubPool ) return nullptr;
        }

        if ( stubOffset + STUB_SIZE > POOL_SIZE ) return nullptr;

        uint8_t* stub = stubPool + stubOffset;
        stubOffset += STUB_SIZE;

        int idx = 0;
        // mov r10, rcx
        stub[idx++] = 0x4C; stub[idx++] = 0x8B; stub[idx++] = 0xD1;
        // mov eax, SSN
        stub[idx++] = 0xB8;
        *reinterpret_cast<uint32_t*>( stub + idx ) = ssn; idx += 4;
        // syscall
        stub[idx++] = 0x0F; stub[idx++] = 0x05;
        // ret
        stub[idx++] = 0xC3;

        return stub;
    }

    // NtProtectVirtualMemory syscall wrapper.
    using fnNtProtectVirtualMemory = NTSTATUS( __stdcall* )(
        HANDLE ProcessHandle, PVOID* BaseAddress, PSIZE_T RegionSize,
        ULONG NewProtect, PULONG OldProtect );

    inline NTSTATUS SyscallNtProtectVirtualMemory(
        HANDLE hProcess, PVOID* baseAddr, PSIZE_T regionSize,
        ULONG newProtect, PULONG oldProtect )
    {
        static auto fn = reinterpret_cast<fnNtProtectVirtualMemory>(
            BuildSyscallStub( GetSyscallNumber( "NtProtectVirtualMemory" ) ) );
        if ( !fn ) return -1;
        return fn( hProcess, baseAddr, regionSize, newProtect, oldProtect );
    }

    // NtSetInformationThread — used to hide thread from debugger.
    using fnNtSetInformationThread = NTSTATUS( __stdcall* )(
        HANDLE ThreadHandle, ULONG ThreadInformationClass,
        PVOID ThreadInformation, ULONG ThreadInformationLength );

    inline NTSTATUS SyscallHideThread( HANDLE hThread )
    {
        static auto fn = reinterpret_cast<fnNtSetInformationThread>(
            BuildSyscallStub( GetSyscallNumber( "NtSetInformationThread" ) ) );
        if ( !fn ) return -1;
        constexpr ULONG ThreadHideFromDebugger = 0x11;
        return fn( hThread, ThreadHideFromDebugger, nullptr, 0 );
    }

    // NtQueryInformationProcess — debug detection.
    using fnNtQueryInformationProcess = NTSTATUS( __stdcall* )(
        HANDLE ProcessHandle, ULONG ProcessInformationClass,
        PVOID ProcessInformation, ULONG ProcessInformationLength, PULONG ReturnLength );

    inline bool IsBeingDebugged_NtQuery()
    {
        static auto fn = reinterpret_cast<fnNtQueryInformationProcess>(
            BuildSyscallStub( GetSyscallNumber( "NtQueryInformationProcess" ) ) );
        if ( !fn ) return false;

        // ProcessDebugPort = 7
        ULONG_PTR debugPort = 0;
        NTSTATUS status = fn( GetCurrentProcess(), 7, &debugPort, sizeof( debugPort ), nullptr );
        return ( status == 0 && debugPort != 0 );
    }

    // NtQuerySystemInformation — for additional checks.
    using fnNtQuerySystemInformation = NTSTATUS( __stdcall* )(
        ULONG SystemInformationClass, PVOID SystemInformation,
        ULONG SystemInformationLength, PULONG ReturnLength );

    inline fnNtQuerySystemInformation GetNtQuerySystemInformation()
    {
        static auto fn = reinterpret_cast<fnNtQuerySystemInformation>(
            BuildSyscallStub( GetSyscallNumber( "NtQuerySystemInformation" ) ) );
        return fn;
    }
}
