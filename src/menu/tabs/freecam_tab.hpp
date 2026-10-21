#pragma once

#include <imgui.h>
#include <features/freecam.hpp>

namespace Menu::Tabs
{
    inline void RenderFreecam()
    {
        auto& fc = Features::Freecam::Get();

        ImGui::SeparatorText( "Freecam" );

        ImGui::Checkbox( "Enable Freecam", &fc.Enabled );
        ImGui::SliderFloat( "Speed", &fc.Speed, 0.1f, 10.f, "%.1f" );
        ImGui::SliderFloat( "Fast Multiplier", &fc.FastMult, 1.f, 10.f, "%.1fx" );
        ImGui::SliderFloat( "Slow Multiplier", &fc.SlowMult, 0.05f, 1.f, "%.2fx" );
        ImGui::Checkbox( "TP to Camera on Exit", &fc.TPOnExit );

        ImGui::Spacing();
        ImGui::SeparatorText( "Controls" );

        ImGui::BulletText( "Toggle: F3 (default)" );
        ImGui::BulletText( "W/A/S/D - Move forward/left/back/right" );
        ImGui::BulletText( "Mouse - Look around" );
        ImGui::BulletText( "Space - Up  |  X - Down" );
        ImGui::BulletText( "Shift - Fast  |  Ctrl - Slow" );
    }
}
