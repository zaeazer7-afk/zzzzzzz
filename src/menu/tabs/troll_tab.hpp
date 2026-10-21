#pragma once

#include <imgui.h>
#include <features/troll.hpp>
#include <features/network.hpp>

namespace Menu::Tabs
{
    inline void RenderTroll()
    {
        auto& troll = Features::Troll::Get();
        auto& net   = Features::Network::Get();

        ImGui::SeparatorText( "Target Player" );

        static int selectedPlayer = -1;
        static int selectedServerId = 0;
        static char selectedName[64] = "";

        if ( ImGui::Button( "Refresh Players" ) )
            net.DumpPlayers();

        if ( !net.m_playerList.empty() )
        {
            if ( ImGui::BeginCombo( "Player", selectedName[0] ? selectedName : "Select..." ) )
            {
                for ( int i = 0; i < static_cast<int>( net.m_playerList.size() ); ++i )
                {
                    auto& p = net.m_playerList[i];
                    char label[128];
                    snprintf( label, sizeof( label ), "[%d] %s", p.serverId, p.name.c_str() );
                    if ( ImGui::Selectable( label, selectedPlayer == i ) )
                    {
                        selectedPlayer = i;
                        selectedServerId = p.serverId;
                        strncpy( selectedName, p.name.c_str(), sizeof( selectedName ) - 1 );
                    }
                }
                ImGui::EndCombo();
            }
        }
        else
        {
            ImGui::TextDisabled( "Click Refresh to load players." );
        }

        if ( selectedServerId <= 0 )
        {
            ImGui::TextDisabled( "Select a player to use troll actions." );
            return;
        }

        ImGui::Text( "Target: [%d] %s", selectedServerId, selectedName );

        ImGui::Spacing();
        ImGui::SeparatorText( "Actions" );

        float btnW = 130.f;
        if ( ImGui::Button( "Explode", ImVec2( btnW, 0 ) ) )
            troll.ExplodePlayer( selectedServerId );
        ImGui::SameLine();
        if ( ImGui::Button( "Cage", ImVec2( btnW, 0 ) ) )
            troll.CagePlayer( selectedServerId );
        ImGui::SameLine();
        if ( ImGui::Button( "Attach Prop", ImVec2( btnW, 0 ) ) )
            troll.AttachProp( selectedServerId );

        if ( ImGui::Button( "Bring", ImVec2( btnW, 0 ) ) )
            troll.BringPlayer( selectedServerId );
        ImGui::SameLine();
        if ( ImGui::Button( "TP To", ImVec2( btnW, 0 ) ) )
            troll.TPToPlayer( selectedServerId );
        ImGui::SameLine();
        if ( ImGui::Button( "Spectate", ImVec2( btnW, 0 ) ) )
            troll.SpectatePlayer( selectedServerId );

        if ( ImGui::Button( "Stop Spectate", ImVec2( btnW, 0 ) ) )
            troll.StopSpectate();
        ImGui::SameLine();
        if ( ImGui::Button( "Clone", ImVec2( btnW, 0 ) ) )
            troll.ClonePlayer( selectedServerId );
        ImGui::SameLine();
        if ( ImGui::Button( "Ragdoll", ImVec2( btnW, 0 ) ) )
            troll.ForceRagdoll( selectedServerId );

        ImGui::Spacing();
        ImGui::SeparatorText( "Area Actions" );

        if ( ImGui::Button( "Kill Nearby Peds", ImVec2( btnW, 0 ) ) )
            troll.KillNearbyPeds();
        ImGui::SameLine();
        if ( ImGui::Button( "Explode Vehicles", ImVec2( btnW, 0 ) ) )
            troll.ExplodeAllVehicles();

        ImGui::Spacing();
        ImGui::SeparatorText( "Spam" );

        if ( ImGui::Button( "Sound Spam", ImVec2( btnW, 0 ) ) )
            troll.SoundSpam( selectedServerId );
        ImGui::SameLine();
        if ( ImGui::Button( "Money Drop", ImVec2( btnW, 0 ) ) )
            troll.MoneyDrop( selectedServerId );

        static char fakeMsg[256] = "";
        ImGui::InputText( "Fake Message", fakeMsg, sizeof( fakeMsg ) );
        ImGui::SameLine();
        if ( ImGui::Button( "Send" ) && fakeMsg[0] )
            troll.FakeMessage( fakeMsg );
    }
}
