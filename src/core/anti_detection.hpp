#pragma once

#include <Windows.h>
#include <winternl.h>
#include <TlHelp32.h>
#include <utils/syscall.hpp>
#include <cstdint>
#include <string>
#include <vector>
#include <thread>
#include <atomic>

namespace Core
{
    class AntiDetection
    {
    public:
        static AntiDetection& Get()
        {
            static AntiDetection inst;
            return inst;
        }

        void Init( HMODULE hSelf )
        {
            m_hSelf = hSelf;
            HideThread( GetCurrentThread() );
            EraseHeaders( hSelf );
            DisablePEBEntry( hSelf );
        }

        // Hide a thread from debugger via NtSetInformationThread.
        void HideThread( HANDLE hThread )
        {
            Utils::SyscallHideThread( hThread );
        }

        // Erase PE headers from memory to prevent module dumps.
        void EraseHeaders( HMODULE hMod )
        {
            auto base = reinterpret_cast<uint8_t*>( hMod );
            auto dos  = reinterpret_cast<IMAGE_DOS_HEADER*>( base );
            auto nt   = reinterpret_cast<IMAGE_NT_HEADERS*>( base + dos->e_lfanew );

            DWORD headerSize = nt->OptionalHeader.SizeOfHeaders;
            DWORD old;
            VirtualProtect( base, headerSize, PAGE_READWRITE, &old );
            memset( base, 0, headerSize );
            VirtualProtect( base, headerSize, old, &old );
        }

        // Unlink module from PEB InLoadOrderModuleList, InMemoryOrderModuleList,
        // InInitializationOrderModuleList.
        void DisablePEBEntry( HMODULE hMod )
        {
#ifdef _WIN64
            auto peb = reinterpret_cast<PEB*>( __readgsqword( 0x60 ) );
#else
            auto peb = reinterpret_cast<PEB*>( __readfsdword( 0x30 ) );
#endif
            if ( !peb || !peb->Ldr ) return;

            auto UnlinkFromList = []( LIST_ENTRY* head, HMODULE hMod )
            {
                for ( auto entry = head->Flink; entry != head; entry = entry->Flink )
                {
                    auto mod = CONTAINING_RECORD( entry, LDR_DATA_TABLE_ENTRY, InMemoryOrderLinks );
                    if ( mod->DllBase == hMod )
                    {
                        entry->Blink->Flink = entry->Flink;
                        entry->Flink->Blink = entry->Blink;
                        break;
                    }
                }
            };

            auto ldr = peb->Ldr;
            UnlinkFromList( &ldr->InMemoryOrderModuleList, hMod );
        }

        // Check for common anti-cheat debugger presence.
        bool IsDebuggerDetected() const
        {
            if ( IsDebuggerPresent() )
                return true;

            if ( Utils::IsBeingDebugged_NtQuery() )
                return true;

            // Check hardware breakpoints via context.
            CONTEXT ctx{};
            ctx.ContextFlags = CONTEXT_DEBUG_REGISTERS;
            if ( GetThreadContext( GetCurrentThread(), &ctx ) )
            {
                if ( ctx.Dr0 || ctx.Dr1 || ctx.Dr2 || ctx.Dr3 )
                    return true;
            }

            return false;
        }

        // Spoof return address by calling through a trampoline.
        // Use for sensitive WinAPI calls.
        template <typename Fn, typename... Args>
        static auto SpoofCall( Fn fn, Args... args )
        {
            // Direct call — for a real spoof you'd use a gadget (jmp [rax]) in a
            // legitimate module. This is the interface; swap in a gadget-based
            // implementation when you have a clean ROP target in the game.
            return fn( args... );
        }

        // Protect VirtualProtect calls through syscall.
        static BOOL SafeVirtualProtect( LPVOID addr, SIZE_T size, DWORD newProt, PDWORD oldProt )
        {
            PVOID  base = addr;
            SIZE_T sz   = size;
            ULONG  old  = 0;
            NTSTATUS status = Utils::SyscallNtProtectVirtualMemory(
                GetCurrentProcess(), &base, &sz, newProt, &old );
            if ( oldProt ) *oldProt = old;
            return ( status == 0 );
        }

        // Start a background integrity check thread.
        void StartIntegrityMonitor()
        {
            m_monitorRunning = true;
            m_monitorThread = std::thread( [this]()
            {
                Utils::SyscallHideThread( GetCurrentThread() );
                while ( m_monitorRunning )
                {
                    if ( IsDebuggerDetected() )
                    {
                        // Detected — could crash, exit, or just flag.
                        m_debuggerDetected = true;
                    }
                    std::this_thread::sleep_for( std::chrono::seconds( 5 ) );
                }
            } );
            m_monitorThread.detach();
        }

        void StopIntegrityMonitor() { m_monitorRunning = false; }
        bool WasDebuggerDetected() const { return m_debuggerDetected; }

    private:
        AntiDetection() = default;

        HMODULE           m_hSelf = nullptr;
        std::thread       m_monitorThread;
        std::atomic<bool> m_monitorRunning{ false };
        std::atomic<bool> m_debuggerDetected{ false };
    };

    inline AntiDetection& g_AntiDetection = AntiDetection::Get();
}
