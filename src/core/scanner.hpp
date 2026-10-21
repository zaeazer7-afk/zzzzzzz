#pragma once

#include <Windows.h>
#include <vector>
#include <string>
#include <cstdint>
#include <Psapi.h>

#pragma comment(lib, "psapi.lib")

namespace Core
{
    class Scanner
    {
    public:

        struct ModuleInfo
        {
            uintptr_t Base;
            size_t    Size;
            char      Name[ MAX_PATH ];
        };

        static std::vector<ModuleInfo> GetModules()
        {
            std::vector<ModuleInfo> out;
            HMODULE mods[ 512 ];
            DWORD needed = 0;
            if ( !EnumProcessModules( GetCurrentProcess(), mods, sizeof( mods ), &needed ) )
                return out;

            const DWORD count = needed / sizeof( HMODULE );
            for ( DWORD i = 0; i < count; i++ )
            {
                MODULEINFO mi{};
                if ( !GetModuleInformation( GetCurrentProcess(), mods[i], &mi, sizeof( mi ) ) )
                    continue;
                ModuleInfo m{};
                m.Base = reinterpret_cast<uintptr_t>( mi.lpBaseOfDll );
                m.Size = mi.SizeOfImage;
                GetModuleBaseNameA( GetCurrentProcess(), mods[i], m.Name, MAX_PATH );
                out.push_back( m );
            }
            return out;
        }

        static ModuleInfo FindModule( const char* name )
        {
            auto mods = GetModules();
            for ( auto& m : mods )
            {
                if ( _stricmp( m.Name, name ) == 0 )
                    return m;
            }
            return {};
        }

        // IDA-style pattern scan: "48 8B 05 ?? ?? ?? ?? 48 85 C0"
        static uintptr_t FindPattern( uintptr_t base, size_t size, const char* pattern )
        {
            auto bytes = ParsePattern( pattern );
            if ( bytes.empty() ) return 0;

            const uint8_t* data = reinterpret_cast<const uint8_t*>( base );
            const size_t   patLen = bytes.size();

            for ( size_t i = 0; i <= size - patLen; i++ )
            {
                bool found = true;
                for ( size_t j = 0; j < patLen; j++ )
                {
                    if ( bytes[j].Wildcard ) continue;
                    if ( data[i + j] != bytes[j].Value ) { found = false; break; }
                }
                if ( found ) return base + i;
            }
            return 0;
        }

        static uintptr_t FindPattern( const char* moduleName, const char* pattern )
        {
            auto mod = FindModule( moduleName );
            if ( !mod.Base ) return 0;
            return FindPattern( mod.Base, mod.Size, pattern );
        }

        // Resolve a RIP-relative address at offset within the matched pattern.
        static uintptr_t ResolveRIP( uintptr_t addr, int ripOffset )
        {
            if ( !addr ) return 0;
            int32_t rel = *reinterpret_cast<int32_t*>( addr + ripOffset );
            return addr + ripOffset + 4 + rel;
        }

    private:

        struct PatByte { uint8_t Value; bool Wildcard; };

        static std::vector<PatByte> ParsePattern( const char* pat )
        {
            std::vector<PatByte> out;
            const char* p = pat;
            while ( *p )
            {
                while ( *p == ' ' ) p++;
                if ( !*p ) break;
                if ( *p == '?' )
                {
                    out.push_back( { 0, true } );
                    p++;
                    if ( *p == '?' ) p++;
                }
                else
                {
                    char hex[3] = { p[0], p[1], 0 };
                    out.push_back( { (uint8_t)strtoul( hex, nullptr, 16 ), false } );
                    p += 2;
                }
            }
            return out;
        }
    };
}
