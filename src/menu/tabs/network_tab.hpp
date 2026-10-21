#pragma once

#include <imgui/imgui.h>
#include <features/network.hpp>

namespace Menu::Tabs
{
    inline void RenderNetwork()
    {
        auto& net = Features::Network::Get();

        ImGui::SeparatorText( "Event Logger" );

        ImGui::Checkbox( "Event Logger", &net.EventLoggerEnabled );
        if ( !net.m_eventLog.empty() )
        {
            ImGui::SameLine();
            if ( ImGui::SmallButton( "Clear Log" ) )
                net.m_eventLog.clear();

            ImGui::BeginChild( "EventLog", ImVec2( 0, 120 ), true );
            for ( auto& e : net.m_eventLog )
                ImGui::TextWrapped( "%s", e.c_str() );
            if ( ImGui::GetScrollY() >= ImGui::GetScrollMaxY() )
                ImGui::SetScrollHereY( 1.f );
            ImGui::EndChild();
        }

        ImGui::Spacing();
        ImGui::SeparatorText( "Information" );

        if ( ImGui::Button( "Dump Resources" ) )
            net.DumpResources();
        ImGui::SameLine();
        if ( ImGui::Button( "Dump Players" ) )
            net.DumpPlayers();
        ImGui::SameLine();
        if ( ImGui::Button( "Dump Server Info" ) )
            net.DumpServerInfo();

        if ( !net.m_serverInfo.empty() )
            ImGui::Text( "Server: %s", net.m_serverInfo.c_str() );

        if ( !net.m_resources.empty() )
        {
            ImGui::Text( "Resources (%d):", static_cast<int>( net.m_resources.size() ) );
            ImGui::BeginChild( "Resources", ImVec2( 0, 100 ), true );
            for ( auto& r : net.m_resources )
                ImGui::Text( "%s", r.c_str() );
            ImGui::EndChild();
        }

        if ( !net.m_playerList.empty() )
        {
            ImGui::Text( "Players (%d):", static_cast<int>( net.m_playerList.size() ) );
            ImGui::BeginChild( "PlayerList", ImVec2( 0, 120 ), true );
            if ( ImGui::BeginTable( "PlayersTable", 5, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg ) )
            {
                ImGui::TableSetupColumn( "Name" );
                ImGui::TableSetupColumn( "ID" );
                ImGui::TableSetupColumn( "HP" );
                ImGui::TableSetupColumn( "Armor" );
                ImGui::TableSetupColumn( "Pos" );
                ImGui::TableHeadersRow();

                for ( auto& p : net.m_playerList )
                {
                    ImGui::TableNextRow();
                    ImGui::TableSetColumnIndex( 0 ); ImGui::Text( "%s", p.name.c_str() );
                    ImGui::TableSetColumnIndex( 1 ); ImGui::Text( "%d", p.serverId );
                    ImGui::TableSetColumnIndex( 2 ); ImGui::Text( "%d", p.health );
                    ImGui::TableSetColumnIndex( 3 ); ImGui::Text( "%d", p.armor );
                    ImGui::TableSetColumnIndex( 4 ); ImGui::Text( "%.0f, %.0f", p.x, p.y );
                }
                ImGui::EndTable();
            }
            ImGui::EndChild();
        }

        ImGui::Spacing();
        ImGui::SeparatorText( "Chat Spam" );

        ImGui::Checkbox( "Enable Chat Spam", &net.ChatSpamEnabled );
        ImGui::InputText( "Message", net.ChatSpamMessage, sizeof( net.ChatSpamMessage ) );
        ImGui::SliderFloat( "Interval (s)", &net.ChatSpamInterval, 0.5f, 10.f, "%.1f" );
    }
}
