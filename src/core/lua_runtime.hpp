#pragma once

#include "scanner.hpp"
#include <cstdint>
#include <string>
#include <vector>
#include <mutex>

namespace Core
{
    using lua_State = void;
    using lua_CFunction = int( * )( lua_State* );

    using fn_luaL_newstate    = lua_State * ( * )( );
    using fn_luaL_loadstring  = int( * )( lua_State*, const char* );
    using fn_luaL_loadbufferx = int( * )( lua_State*, const char*, size_t, const char*, const char* );
    using fn_lua_pcall        = int( * )( lua_State*, int, int, int );
    using fn_lua_pcallk       = int( * )( lua_State*, int, int, int, intptr_t, lua_CFunction );
    using fn_lua_settop       = void( * )( lua_State*, int );
    using fn_lua_gettop       = int( * )( lua_State* );
    using fn_lua_tolstring    = const char* ( * )( lua_State*, int, size_t* );
    using fn_lua_getglobal    = int( * )( lua_State*, const char* );
    using fn_lua_setglobal    = void( * )( lua_State*, const char* );
    using fn_lua_pushstring   = const char* ( * )( lua_State*, const char* );
    using fn_lua_pushinteger  = void( * )( lua_State*, long long );
    using fn_lua_pushnumber   = void( * )( lua_State*, double );
    using fn_lua_pushboolean  = void( * )( lua_State*, int );
    using fn_lua_pushnil      = void( * )( lua_State* );
    using fn_lua_type         = int( * )( lua_State*, int );
    using fn_lua_typename     = const char* ( * )( lua_State*, int );
    using fn_lua_toboolean    = int( * )( lua_State*, int );
    using fn_lua_tonumberx    = double( * )( lua_State*, int, int* );
    using fn_lua_tointegerx   = long long( * )( lua_State*, int, int* );
    using fn_lua_createtable  = void( * )( lua_State*, int, int );
    using fn_lua_settable     = void( * )( lua_State*, int );
    using fn_lua_rawseti      = void( * )( lua_State*, int, long long );
    using fn_lua_next         = int( * )( lua_State*, int );
    using fn_luaL_ref         = int( * )( lua_State*, int );
    using fn_luaL_unref       = void( * )( lua_State*, int, int );

    struct LuaFunctions
    {
        fn_luaL_loadstring  luaL_loadstring  = nullptr;
        fn_luaL_loadbufferx luaL_loadbufferx = nullptr;
        fn_lua_pcallk       lua_pcallk       = nullptr;
        fn_lua_settop       lua_settop       = nullptr;
        fn_lua_gettop       lua_gettop       = nullptr;
        fn_lua_tolstring    lua_tolstring     = nullptr;
        fn_lua_getglobal    lua_getglobal    = nullptr;
        fn_lua_setglobal    lua_setglobal    = nullptr;
        fn_lua_pushstring   lua_pushstring   = nullptr;
        fn_lua_pushinteger  lua_pushinteger  = nullptr;
        fn_lua_pushnumber   lua_pushnumber   = nullptr;
        fn_lua_pushboolean  lua_pushboolean  = nullptr;
        fn_lua_pushnil      lua_pushnil      = nullptr;
        fn_lua_type         lua_type         = nullptr;
        fn_lua_typename     lua_typename     = nullptr;
        fn_lua_toboolean    lua_toboolean    = nullptr;
        fn_lua_tonumberx    lua_tonumberx    = nullptr;
        fn_lua_tointegerx   lua_tointegerx   = nullptr;
        fn_lua_createtable  lua_createtable  = nullptr;
        fn_lua_settable     lua_settable     = nullptr;
        fn_lua_rawseti      lua_rawseti      = nullptr;
        fn_lua_next         lua_next         = nullptr;
        fn_luaL_ref         luaL_ref         = nullptr;
        fn_luaL_unref       luaL_unref       = nullptr;

        int pcall( lua_State* L, int nargs, int nresults, int errfunc ) const
        {
            if ( lua_pcallk )
                return lua_pcallk( L, nargs, nresults, errfunc, 0, nullptr );
            return -1;
        }

        const char* tostring( lua_State* L, int idx ) const
        {
            if ( lua_tolstring )
                return lua_tolstring( L, idx, nullptr );
            return nullptr;
        }

        bool IsValid() const
        {
            return luaL_loadstring && lua_pcallk && lua_settop && lua_gettop && lua_tolstring;
        }
    };

    struct FxResource
    {
        char pad_000[ 0x40 ];
    };

    struct FxResourceImpl : FxResource
    {
        std::string m_name;
    };

    struct FxNetLibrary
    {
        char        pad_000[ 0xF8 ];
        std::string m_currentServerUrl;
    };

    class LuaRuntime
    {
    public:

        static LuaRuntime& Get()
        {
            static LuaRuntime inst;
            return inst;
        }

        bool Init()
        {
            std::lock_guard<std::mutex> lk( m_mtx );
            if ( m_ready ) return true;

            const char* luaModuleNames[] = {
                "citizen-scripting-lua54.dll",
                "citizen-scripting-lua.dll",
                "lua54.dll",
                "lua5.4.dll"
            };

            HMODULE hLua = nullptr;
            for ( auto name : luaModuleNames )
            {
                hLua = GetModuleHandleA( name );
                if ( hLua ) break;
            }
            if ( !hLua ) return false;

            m_luaModule = reinterpret_cast<uintptr_t>( hLua );

            auto Exp = [&]( const char* name ) -> void* {
                return reinterpret_cast<void*>( GetProcAddress( hLua, name ) );
            };

            m_fn.luaL_loadstring  = (fn_luaL_loadstring)  Exp( "luaL_loadstring" );
            m_fn.luaL_loadbufferx = (fn_luaL_loadbufferx) Exp( "luaL_loadbufferx" );
            m_fn.lua_pcallk       = (fn_lua_pcallk)       Exp( "lua_pcallk" );
            m_fn.lua_settop       = (fn_lua_settop)       Exp( "lua_settop" );
            m_fn.lua_gettop       = (fn_lua_gettop)       Exp( "lua_gettop" );
            m_fn.lua_tolstring    = (fn_lua_tolstring)     Exp( "lua_tolstring" );
            m_fn.lua_getglobal    = (fn_lua_getglobal)    Exp( "lua_getglobal" );
            m_fn.lua_setglobal    = (fn_lua_setglobal)    Exp( "lua_setglobal" );
            m_fn.lua_pushstring   = (fn_lua_pushstring)   Exp( "lua_pushstring" );
            m_fn.lua_pushinteger  = (fn_lua_pushinteger)  Exp( "lua_pushinteger" );
            m_fn.lua_pushnumber   = (fn_lua_pushnumber)   Exp( "lua_pushnumber" );
            m_fn.lua_pushboolean  = (fn_lua_pushboolean)  Exp( "lua_pushboolean" );
            m_fn.lua_pushnil      = (fn_lua_pushnil)      Exp( "lua_pushnil" );
            m_fn.lua_type         = (fn_lua_type)         Exp( "lua_type" );
            m_fn.lua_typename     = (fn_lua_typename)     Exp( "lua_typename" );
            m_fn.lua_toboolean    = (fn_lua_toboolean)    Exp( "lua_toboolean" );
            m_fn.lua_tonumberx    = (fn_lua_tonumberx)    Exp( "lua_tonumberx" );
            m_fn.lua_tointegerx   = (fn_lua_tointegerx)   Exp( "lua_tointegerx" );
            m_fn.lua_createtable  = (fn_lua_createtable)  Exp( "lua_createtable" );
            m_fn.lua_settable     = (fn_lua_settable)     Exp( "lua_settable" );
            m_fn.lua_rawseti      = (fn_lua_rawseti)      Exp( "lua_rawseti" );
            m_fn.lua_next         = (fn_lua_next)         Exp( "lua_next" );
            m_fn.luaL_ref         = (fn_luaL_ref)         Exp( "luaL_ref" );
            m_fn.luaL_unref       = (fn_luaL_unref)       Exp( "luaL_unref" );

            if ( !m_fn.IsValid() ) return false;

            HMODULE hRes = GetModuleHandleA( "citizen-resources-core.dll" );
            if ( !hRes ) return false;
            m_resourceModule = reinterpret_cast<uintptr_t>( hRes );

            HMODULE hNet = GetModuleHandleA( "gta-net-five.dll" );
            if ( hNet )
                m_netModule = reinterpret_cast<uintptr_t>( hNet );

            m_ready = true;
            return true;
        }

        bool IsReady() const { return m_ready; }

        const LuaFunctions& Fn() const { return m_fn; }

        lua_State* GetActiveLuaState() const
        {
            if ( !m_ready ) return nullptr;

            const char* scriptModNames[] = {
                "citizen-scripting-lua54.dll",
                "citizen-scripting-lua.dll"
            };

            const char* patterns[] = {
                "48 8B 05 ?? ?? ?? ?? 48 85 C0 74 ?? 48 8B 48",
                "48 8B 05 ?? ?? ?? ?? 48 85 C0 74",
                "48 8B 0D ?? ?? ?? ?? 48 85 C9 74",
                "48 8B 05 ?? ?? ?? ?? 48 85 C0 0F 84",
                "48 8B 05 ?? ?? ?? ?? 48 8B 08 48 85 C9",
            };

            for ( auto modName : scriptModNames )
            {
                auto mod = Scanner::FindModule( modName );
                if ( !mod.Base ) continue;

                for ( auto pat : patterns )
                {
                    uintptr_t addr = Scanner::FindPattern( mod.Base, mod.Size, pat );
                    if ( !addr ) continue;

                    uintptr_t pRuntimePtr = Scanner::ResolveRIP( addr, 3 );
                    if ( !pRuntimePtr ) continue;

                    uintptr_t runtime = *reinterpret_cast<uintptr_t*>( pRuntimePtr );
                    if ( !runtime ) continue;

                    if ( IsBadReadPtr( reinterpret_cast<void*>( runtime ), 8 ) ) continue;

                    const int stateOffsets[] = {
                        0x48, 0x50, 0x58, 0x60, 0x68, 0x70, 0x78
                    };

                    for ( int off : stateOffsets )
                    {
                        if ( IsBadReadPtr( reinterpret_cast<void*>( runtime + off ), 8 ) ) continue;
                        uintptr_t candidate = *reinterpret_cast<uintptr_t*>( runtime + off );
                        if ( !candidate ) continue;
                        if ( IsBadReadPtr( reinterpret_cast<void*>( candidate ), 64 ) ) continue;

                        __try
                        {
                            int top = m_fn.lua_gettop( reinterpret_cast<lua_State*>( candidate ) );
                            if ( top >= 0 && top < 1000 )
                            {
                                m_cachedState = reinterpret_cast<lua_State*>( candidate );
                                m_cachedStateOffset = off;
                                return m_cachedState;
                            }
                        }
                        __except ( EXCEPTION_EXECUTE_HANDLER ) {}
                    }
                }
            }
            return nullptr;
        }

        lua_State* GetCachedState() const
        {
            if ( m_cachedState && m_cachedStateOffset >= 0 )
            {
                __try
                {
                    int top = m_fn.lua_gettop( m_cachedState );
                    if ( top >= 0 && top < 1000 )
                        return m_cachedState;
                }
                __except ( EXCEPTION_EXECUTE_HANDLER ) {}

                m_cachedState = nullptr;
                m_cachedStateOffset = -1;
            }
            return GetActiveLuaState();
        }

        std::string GetServerUrl() const
        {
            if ( !m_netModule ) return "";

            auto mod = Scanner::FindModule( "gta-net-five.dll" );
            if ( !mod.Base ) return "";

            const char* netPatterns[] = {
                "48 8B 0D ?? ?? ?? ?? E8 ?? ?? ?? ?? 48 8B D8",
                "48 8B 0D ?? ?? ?? ?? 48 85 C9 74 ?? E8",
                "48 8B 05 ?? ?? ?? ?? 48 85 C0 74 ?? 48 8D 88",
                "48 8B 0D ?? ?? ?? ?? E8 ?? ?? ?? ?? 84 C0",
            };

            for ( auto pat : netPatterns )
            {
                uintptr_t addr = Scanner::FindPattern( mod.Base, mod.Size, pat );
                if ( !addr ) continue;

                uintptr_t pNetLib = Scanner::ResolveRIP( addr, 3 );
                if ( !pNetLib ) continue;

                uintptr_t netLib = *reinterpret_cast<uintptr_t*>( pNetLib );
                if ( !netLib || IsBadReadPtr( reinterpret_cast<void*>( netLib ), 0x200 ) ) continue;

                const int urlOffsets[] = { 0xF8, 0x100, 0x108, 0x110, 0x118, 0x120 };
                for ( int off : urlOffsets )
                {
                    __try
                    {
                        auto* str = reinterpret_cast<std::string*>( netLib + off );
                        if ( str->size() > 5 && str->size() < 256 &&
                            ( str->find( "http" ) == 0 || str->find( "cfx.re" ) != std::string::npos ) )
                        {
                            return *str;
                        }
                    }
                    __except ( EXCEPTION_EXECUTE_HANDLER ) {}
                }
            }
            return "";
        }

    private:

        LuaRuntime() = default;

        std::mutex     m_mtx;
        bool           m_ready         = false;
        uintptr_t      m_luaModule     = 0;
        uintptr_t      m_resourceModule = 0;
        uintptr_t      m_netModule     = 0;
        LuaFunctions   m_fn;

        mutable lua_State* m_cachedState       = nullptr;
        mutable int        m_cachedStateOffset = -1;
    };

    inline LuaRuntime& g_LuaRuntime = LuaRuntime::Get();
}
