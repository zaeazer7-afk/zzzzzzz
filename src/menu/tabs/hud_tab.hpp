#pragma once

#include <imgui/imgui.h>
#include <features/hud.hpp>

namespace Menu::Tabs
{
    inline void RenderHUD()
    {
        auto& h = Features::HUD::Get();

        ImGui::SeparatorText( "Overlays" );

        ImGui::Checkbox( "Watermark", &h.Watermark );
        ImGui::Checkbox( "FPS Counter", &h.FPSCounter );
        ImGui::Checkbox( "Coordinates", &h.CoordDisplay );
        ImGui::Checkbox( "Speed Display", &h.SpeedDisplay );
        ImGui::Checkbox( "Session Info", &h.SessionInfo );
        ImGui::Checkbox( "Enlarge Minimap", &h.MinimapEnlarge );

        ImGui::Spacing();
        ImGui::SeparatorText( "Crosshair" );

        ImGui::Checkbox( "Enable Crosshair", &h.CrosshairEnabled );

        if ( h.CrosshairEnabled )
        {
            const char* styles[] = { "None", "Cross", "Circle", "Dot", "Cross + Dot" };
            ImGui::Combo( "Style", &h.CrosshairStyle, styles, 5 );
            ImGui::SliderFloat( "Size", &h.CrosshairSize, 2.f, 30.f, "%.0f" );
            ImGui::ColorEdit4( "Color", &h.CrosshairColor.x, ImGuiColorEditFlags_NoInputs );
        }
    }
}
