#pragma once

#include <imgui.h>
#include <features/teleport.hpp>

namespace Menu::Tabs
{
    inline void RenderTeleport()
    {
        auto& tp = Features::Teleport::Get();

        ImGui::SeparatorText( "Quick Teleport" );

        if ( ImGui::Button( "TP to Waypoint", ImVec2( 200, 0 ) ) )
            tp.TeleportToWaypoint();
        ImGui::SameLine();
        if ( ImGui::Button( "TP Forward", ImVec2( 120, 0 ) ) )
            tp.TeleportForward( 10.f );
        ImGui::SameLine();
        if ( ImGui::Button( "TP Back", ImVec2( 120, 0 ) ) )
            tp.TeleportBack();

        ImGui::Spacing();
        ImGui::SeparatorText( "Custom Coordinates" );

        ImGui::PushItemWidth( 120 );
        ImGui::InputFloat( "X", &tp.CustomX, 1.f, 10.f, "%.2f" );
        ImGui::SameLine();
        ImGui::InputFloat( "Y", &tp.CustomY, 1.f, 10.f, "%.2f" );
        ImGui::SameLine();
        ImGui::InputFloat( "Z", &tp.CustomZ, 1.f, 10.f, "%.2f" );
        ImGui::PopItemWidth();
        if ( ImGui::Button( "Teleport to Coords" ) )
            tp.TeleportToCoords( tp.CustomX, tp.CustomY, tp.CustomZ );

        ImGui::Spacing();
        ImGui::SeparatorText( "Saved Locations" );

        ImGui::InputText( "Name", tp.LocationNameBuf, sizeof( tp.LocationNameBuf ) );
        ImGui::SameLine();
        if ( ImGui::Button( "Save Current" ) && tp.LocationNameBuf[0] )
            tp.SaveLocation( tp.LocationNameBuf );

        if ( !tp.Saved.empty() )
        {
            ImGui::BeginChild( "SavedLocs", ImVec2( 0, 150 ), true );
            for ( int i = 0; i < static_cast<int>( tp.Saved.size() ); ++i )
            {
                auto& loc = tp.Saved[i];
                ImGui::PushID( i );
                ImGui::Text( "%s (%.0f, %.0f, %.0f)", loc.name.c_str(), loc.x, loc.y, loc.z );
                ImGui::SameLine( 350.f );
                if ( ImGui::SmallButton( "TP" ) )
                    tp.TeleportToCoords( loc.x, loc.y, loc.z );
                ImGui::SameLine();
                if ( ImGui::SmallButton( "Del" ) )
                {
                    tp.Saved.erase( tp.Saved.begin() + i );
                    ImGui::PopID();
                    break;
                }
                ImGui::PopID();
            }
            ImGui::EndChild();
        }

        ImGui::Spacing();
        ImGui::SeparatorText( "Teleport History" );

        if ( !tp.History.empty() )
        {
            ImGui::BeginChild( "TPHistory", ImVec2( 0, 120 ), true );
            for ( int i = static_cast<int>( tp.History.size() ) - 1; i >= 0; --i )
            {
                auto& h = tp.History[i];
                ImGui::PushID( 1000 + i );
                ImGui::Text( "%.0f, %.0f, %.0f", h.x, h.y, h.z );
                ImGui::SameLine( 300.f );
                if ( ImGui::SmallButton( "TP" ) )
                    tp.TeleportToCoords( h.x, h.y, h.z );
                ImGui::PopID();
            }
            ImGui::EndChild();
        }
        else
        {
            ImGui::TextDisabled( "No history yet." );
        }

        ImGui::Spacing();
        ImGui::SeparatorText( "Blip Teleport" );

        if ( ImGui::Button( "TP Into Nearest Vehicle" ) )
            tp.TeleportIntoClosestVehicle();
    }
}
