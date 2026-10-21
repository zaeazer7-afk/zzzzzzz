#pragma once

#include <imgui.h>
#include <features/network.hpp>
#include <features/troll.hpp>
#include <features/hud.hpp>
#include <menu/notifications.hpp>
#include <cmath>

namespace Menu::Tabs
{
    inline void RenderPlayers()
    {
        auto& net = Features::Network::Get();
        auto& hud = Features::HUD::Get();

        static int selectedIdx = -1;

        if ( ImGui::Button( "Refresh Player List" ) )
            net.DumpPlayers();

        ImGui::SameLine();
        ImGui::Text( "%d players", static_cast<int>( net.m_playerList.size() ) );

        ImGui::Spacing();

        if ( net.m_playerList.empty() )
        {
            ImGui::TextDisabled( "Click Refresh to load the player list." );
            return;
        }

        ImGui::BeginChild( "PlayerTable", ImVec2( 0, 300 ), true );
        if ( ImGui::BeginTable( "Players", 6,
            ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_Resizable | ImGuiTableFlags_ScrollY ) )
        {
            ImGui::TableSetupColumn( "Name", ImGuiTableColumnFlags_WidthStretch );
            ImGui::TableSetupColumn( "Server ID", ImGuiTableColumnFlags_WidthFixed, 70.f );
            ImGui::TableSetupColumn( "Health", ImGuiTableColumnFlags_WidthFixed, 60.f );
            ImGui::TableSetupColumn( "Armor", ImGuiTableColumnFlags_WidthFixed, 60.f );
            ImGui::TableSetupColumn( "Distance", ImGuiTableColumnFlags_WidthFixed, 80.f );
            ImGui::TableSetupColumn( "Actions", ImGuiTableColumnFlags_WidthFixed, 180.f );
            ImGui::TableHeadersRow();

            for ( int i = 0; i < static_cast<int>( net.m_playerList.size() ); ++i )
            {
                auto& p = net.m_playerList[i];
                ImGui::TableNextRow();
                ImGui::PushID( i );

                bool isSelected = ( selectedIdx == i );

                ImGui::TableSetColumnIndex( 0 );
                if ( ImGui::Selectable( p.name.c_str(), isSelected, ImGuiSelectableFlags_SpanAllColumns ) )
                    selectedIdx = i;

                ImGui::TableSetColumnIndex( 1 );
                ImGui::Text( "%d", p.serverId );

                ImGui::TableSetColumnIndex( 2 );
                ImVec4 hpCol = p.health > 100 ? ImVec4( 0.f, 1.f, 0.f, 1.f ) :
                               p.health > 50  ? ImVec4( 1.f, 1.f, 0.f, 1.f ) :
                                                 ImVec4( 1.f, 0.f, 0.f, 1.f );
                ImGui::TextColored( hpCol, "%d", p.health );

                ImGui::TableSetColumnIndex( 3 );
                ImGui::Text( "%d", p.armor );

                ImGui::TableSetColumnIndex( 4 );
                float dx = p.x - hud.m_playerX;
                float dy = p.y - hud.m_playerY;
                float dist = sqrtf( dx * dx + dy * dy );
                ImGui::Text( "%.0f m", dist );

                ImGui::TableSetColumnIndex( 5 );
                if ( ImGui::SmallButton( "TP To" ) )
                    Features::Troll::Get().TPToPlayer( p.serverId );
                ImGui::SameLine();
                if ( ImGui::SmallButton( "Spectate" ) )
                    Features::Troll::Get().SpectatePlayer( p.serverId );
                ImGui::SameLine();
                if ( ImGui::SmallButton( "Copy" ) )
                {
                    char info[256];
                    snprintf( info, sizeof( info ), "%s [%d] HP:%d Pos:%.0f,%.0f,%.0f",
                        p.name.c_str(), p.serverId, p.health, p.x, p.y, p.z );
                    ImGui::SetClipboardText( info );
                    Menu::g_Notifs.Add( "Copied player info", Menu::NotifType::Info );
                }

                ImGui::PopID();
            }
            ImGui::EndTable();
        }
        ImGui::EndChild();

        if ( selectedIdx >= 0 && selectedIdx < static_cast<int>( net.m_playerList.size() ) )
        {
            auto& sel = net.m_playerList[selectedIdx];
            ImGui::Spacing();
            ImGui::SeparatorText( "Selected Player" );
            ImGui::Text( "Name: %s", sel.name.c_str() );
            ImGui::Text( "Server ID: %d", sel.serverId );
            ImGui::Text( "Health: %d | Armor: %d", sel.health, sel.armor );
            ImGui::Text( "Position: %.1f, %.1f, %.1f", sel.x, sel.y, sel.z );
        }
    }
}
