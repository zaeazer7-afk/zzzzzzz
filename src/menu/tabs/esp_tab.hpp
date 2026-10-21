#pragma once

#include <imgui/imgui.h>
#include <features/esp.hpp>

namespace Menu::Tabs
{
    inline void RenderESP()
    {
        auto& esp = Features::ESP::Get();

        ImGui::SeparatorText( "ESP Settings" );

        ImGui::Checkbox( "Enable ESP", &esp.Enabled );
        ImGui::Spacing();

        ImGui::Checkbox( "Box", &esp.BoxESP );
        ImGui::SameLine( 200.f );
        ImGui::Checkbox( "Name", &esp.NameESP );

        ImGui::Checkbox( "Health Bar", &esp.HealthBar );
        ImGui::SameLine( 200.f );
        ImGui::Checkbox( "Armor Bar", &esp.ArmorBar );

        ImGui::Checkbox( "Distance", &esp.DistanceESP );
        ImGui::SameLine( 200.f );
        ImGui::Checkbox( "Snaplines", &esp.Snaplines );

        ImGui::Checkbox( "Skeleton", &esp.Skeleton );
        ImGui::SameLine( 200.f );
        ImGui::Checkbox( "Vehicle ESP", &esp.VehicleESP );

        if ( esp.Snaplines )
        {
            const char* snapPos[] = { "Top", "Center", "Bottom" };
            ImGui::Combo( "Snapline Origin", &esp.SnaplinePos, snapPos, 3 );
        }

        ImGui::Spacing();
        ImGui::SeparatorText( "Range & Colors" );

        ImGui::SliderFloat( "Max Distance", &esp.MaxDistance, 50.f, 2000.f, "%.0f m" );

        ImGui::ColorEdit4( "Box Color", &esp.BoxColor.x, ImGuiColorEditFlags_NoInputs );
        ImGui::SameLine();
        ImGui::Text( "Box" );

        ImGui::ColorEdit4( "Name Color", &esp.NameColor.x, ImGuiColorEditFlags_NoInputs );
        ImGui::SameLine();
        ImGui::Text( "Name" );

        ImGui::ColorEdit4( "Snapline Color", &esp.SnapColor.x, ImGuiColorEditFlags_NoInputs );
        ImGui::SameLine();
        ImGui::Text( "Snaplines" );

        ImGui::ColorEdit4( "Skeleton Color", &esp.SkeletonColor.x, ImGuiColorEditFlags_NoInputs );
        ImGui::SameLine();
        ImGui::Text( "Skeleton" );
    }
}
