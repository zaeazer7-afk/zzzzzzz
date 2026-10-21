#pragma once

#include "lua_runtime.hpp"
#include "scanner.hpp"

#include <string>
#include <vector>
#include <deque>
#include <mutex>
#include <functional>
#include <atomic>

namespace Core
{
    struct ExecResult
    {
        bool        Ok;
        std::string Error;
        std::string Output;
    };

    struct ExecHistoryEntry
    {
        std::string Label;
        std::string Code;
        bool        Ok;
        std::string Error;
    };

    class Scheduler
    {
    public:

        static Scheduler& Get()
        {
            static Scheduler inst;
            return inst;
        }

        bool IsReady() const
        {
            return g_LuaRuntime.IsReady();
        }

        ExecResult Execute( const char* code, const char* label = "custom" )
        {
            ExecResult res{};
            if ( !code || !code[0] ) { res.Error = "empty code"; return res; }

            auto& rt = g_LuaRuntime;
            if ( !rt.IsReady() )
            {
                if ( !rt.Init() ) { res.Error = "lua runtime not ready"; return res; }
            }

            lua_State* L = rt.GetCachedState();
            if ( !L )
            {
                L = rt.GetActiveLuaState();
                if ( !L ) { res.Error = "no active lua_State"; return res; }
            }

            const auto& fn = rt.Fn();

            std::lock_guard<std::mutex> lk( m_execMtx );

            int topBefore = fn.lua_gettop( L );

            int loadErr = fn.luaL_loadstring( L, code );
            if ( loadErr != 0 )
            {
                const char* err = fn.tostring( L, -1 );
                res.Error = err ? err : "load error";
                fn.lua_settop( L, topBefore );
                PushHistory( label, code, false, res.Error );
                return res;
            }

            int callErr = fn.pcall( L, 0, -1, 0 );
            if ( callErr != 0 )
            {
                const char* err = fn.tostring( L, -1 );
                res.Error = err ? err : "runtime error";
                fn.lua_settop( L, topBefore );
                PushHistory( label, code, false, res.Error );
                return res;
            }

            int topAfter = fn.lua_gettop( L );
            for ( int i = topBefore + 1; i <= topAfter; i++ )
            {
                const char* s = fn.tostring( L, i );
                if ( s )
                {
                    if ( !res.Output.empty() ) res.Output += "\t";
                    res.Output += s;
                }
            }

            fn.lua_settop( L, topBefore );
            res.Ok = true;
            PushHistory( label, code, true, "" );
            return res;
        }

        ExecResult ExecuteSafe( const char* code, const char* label = "custom" )
        {
            std::string wrapped = "local __ok,__err = pcall(function()\n";
            wrapped += code;
            wrapped += "\nend)\nif not __ok then error(__err) end";
            return Execute( wrapped.c_str(), label );
        }

        ExecResult TriggerServerEvent( const char* eventName, const char* argsLua = "" )
        {
            if ( !eventName || !eventName[0] ) return { false, "no event name" };

            std::string code = "TriggerServerEvent('";
            code += eventName;
            code += "'";
            if ( argsLua && argsLua[0] )
            {
                code += ", ";
                code += argsLua;
            }
            code += ")";
            return ExecuteSafe( code.c_str(), eventName );
        }

        ExecResult TriggerClientEvent( const char* eventName, const char* argsLua = "" )
        {
            if ( !eventName || !eventName[0] ) return { false, "no event name" };

            std::string code = "TriggerEvent('";
            code += eventName;
            code += "'";
            if ( argsLua && argsLua[0] )
            {
                code += ", ";
                code += argsLua;
            }
            code += ")";
            return ExecuteSafe( code.c_str(), eventName );
        }

        ExecResult ExecuteCommand( const char* cmd )
        {
            if ( !cmd || !cmd[0] ) return { false, "no command" };
            std::string code = "ExecuteCommand('";
            code += cmd;
            code += "')";
            return ExecuteSafe( code.c_str(), cmd );
        }

        void QueueExecution( const std::string& code, const std::string& label = "queued" )
        {
            std::lock_guard<std::mutex> lk( m_queueMtx );
            m_queue.push_back( { label, code } );
        }

        void ProcessQueue()
        {
            std::lock_guard<std::mutex> lk( m_queueMtx );
            while ( !m_queue.empty() )
            {
                auto& front = m_queue.front();
                ExecuteSafe( front.Code.c_str(), front.Label.c_str() );
                m_queue.pop_front();
            }
        }

        std::deque<ExecHistoryEntry> GetHistory() const
        {
            std::lock_guard<std::mutex> lk( m_histMtx );
            return m_history;
        }

        void ClearHistory()
        {
            std::lock_guard<std::mutex> lk( m_histMtx );
            m_history.clear();
        }

    private:

        Scheduler() = default;

        struct QueueEntry { std::string Label; std::string Code; };

        void PushHistory( const char* label, const char* code, bool ok, const std::string& err )
        {
            std::lock_guard<std::mutex> lk( m_histMtx );
            if ( m_history.size() >= 500 )
                m_history.pop_front();
            m_history.push_back( { label ? label : "", code ? code : "", ok, err } );
        }

        std::mutex                   m_execMtx;
        std::mutex                   m_queueMtx;
        mutable std::mutex           m_histMtx;
        std::deque<QueueEntry>       m_queue;
        std::deque<ExecHistoryEntry> m_history;
    };

    inline Scheduler& g_Scheduler = Scheduler::Get();
}
