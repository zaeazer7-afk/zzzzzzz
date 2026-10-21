#pragma once

#include <core/scheduler.hpp>
#include <utils/xor_string.hpp>
#include <string>
#include <vector>
#include <chrono>

namespace Features
{
    struct PlayerInfo
    {
        std::string name;
        int         serverId = 0;
        int         ped      = 0;
        float       x = 0, y = 0, z = 0;
        int         health   = 0;
        int         armor    = 0;
    };

    class Network
    {
    public:
        static Network& Get()
        {
            static Network inst;
            return inst;
        }

        bool  EventLoggerEnabled = false;
        bool  ChatSpamEnabled    = false;
        float ChatSpamInterval   = 2.f;
        char  ChatSpamMessage[256] = "Trident Internal";

        std::vector<std::string> m_eventLog;
        std::vector<std::string> m_resources;
        std::vector<PlayerInfo>  m_playerList;
        std::string              m_serverInfo;

        void Update()
        {
            if ( ChatSpamEnabled )
            {
                auto now = std::chrono::steady_clock::now();
                float elapsed = std::chrono::duration<float>( now - m_lastSpam ).count();
                if ( elapsed >= ChatSpamInterval )
                {
                    m_lastSpam = now;
                    std::string msg = ChatSpamMessage;
                    std::string code = std::string( R"(TriggerServerEvent('chatMessage', '', {0}, ')" ) + msg + "')";
                    Core::g_Scheduler.Execute( code );
                }
            }
        }

        void DumpResources()
        {
            m_resources.clear();
            std::string code = R"(
                local count = GetNumResources()
                local out = ''
                for i = 0, count - 1 do
                    local name = GetResourceByFindIndex(i)
                    local state = GetResourceState(name)
                    out = out .. name .. ':' .. state .. '|'
                end
                return out
            )";

            auto result = Core::g_Scheduler.Execute( code );
            if ( result.Ok && !result.Output.empty() )
            {
                std::string data = result.Output;
                size_t pos = 0;
                while ( ( pos = data.find( '|' ) ) != std::string::npos )
                {
                    std::string entry = data.substr( 0, pos );
                    if ( !entry.empty() )
                        m_resources.push_back( entry );
                    data.erase( 0, pos + 1 );
                }
            }
        }

        void DumpPlayers()
        {
            m_playerList.clear();
            std::string code = R"(
                local out = ''
                for _, pid in ipairs(GetActivePlayers()) do
                    local ped = GetPlayerPed(pid)
                    local sid = GetPlayerServerId(pid)
                    local name = GetPlayerName(pid) or 'Unknown'
                    local hp = GetEntityHealth(ped)
                    local arm = GetPedArmour(ped)
                    local c = GetEntityCoords(ped)
                    out = out .. name .. ':' .. sid .. ':' .. ped .. ':'
                    out = out .. string.format('%.1f', c.x) .. ':'
                    out = out .. string.format('%.1f', c.y) .. ':'
                    out = out .. string.format('%.1f', c.z) .. ':'
                    out = out .. hp .. ':' .. arm .. '|'
                end
                return out
            )";

            auto result = Core::g_Scheduler.Execute( code );
            if ( result.Ok && !result.Output.empty() )
            {
                std::string data = result.Output;
                size_t pos = 0;
                while ( ( pos = data.find( '|' ) ) != std::string::npos )
                {
                    std::string entry = data.substr( 0, pos );
                    data.erase( 0, pos + 1 );
                    if ( entry.empty() ) continue;

                    PlayerInfo pi;
                    std::vector<std::string> parts;
                    size_t p = 0;
                    while ( ( p = entry.find( ':' ) ) != std::string::npos )
                    {
                        parts.push_back( entry.substr( 0, p ) );
                        entry.erase( 0, p + 1 );
                    }
                    if ( !entry.empty() ) parts.push_back( entry );

                    if ( parts.size() >= 8 )
                    {
                        pi.name     = parts[0];
                        pi.serverId = std::atoi( parts[1].c_str() );
                        pi.ped      = std::atoi( parts[2].c_str() );
                        pi.x        = std::atof( parts[3].c_str() );
                        pi.y        = std::atof( parts[4].c_str() );
                        pi.z        = std::atof( parts[5].c_str() );
                        pi.health   = std::atoi( parts[6].c_str() );
                        pi.armor    = std::atoi( parts[7].c_str() );
                        m_playerList.push_back( pi );
                    }
                }
            }
        }

        void DumpServerInfo()
        {
            std::string code = R"(
                local host = GetConvar('sv_hostname', 'Unknown')
                local maxp = GetConvarInt('sv_maxclients', 32)
                local count = #GetActivePlayers()
                return host .. '|' .. count .. '/' .. maxp
            )";

            auto result = Core::g_Scheduler.Execute( code );
            if ( result.Ok )
                m_serverInfo = result.Output;
        }

        void LogEvent( const std::string& event )
        {
            if ( m_eventLog.size() > 200 )
                m_eventLog.erase( m_eventLog.begin() );
            m_eventLog.push_back( event );
        }

    private:
        Network() = default;
        std::chrono::steady_clock::time_point m_lastSpam{};
    };

    inline Network& g_Network = Network::Get();
}
