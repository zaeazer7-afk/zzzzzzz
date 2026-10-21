#pragma once

#include <imgui/imgui.h>
#include <features/aimbot.hpp>

namespace Menu::Tabs
{
    inline void RenderAimbot()
    {
        auto& aim = Features::Aimbot::Get();

        ImGui::SeparatorText( "Aimbot Settings" );

        ImGui::Checkbox( "Enable Aimbot", &aim.Enabled );
        ImGui::Spacing();

        ImGui::SliderFloat( "FOV", &aim.FOV, 10.f, 800.f, "%.0f px" );
        ImGui::SliderFloat( "Smoothing", &aim.Smoothing, 1.f, 20.f, "%.1f" );

        const char* bones[] = { "Head", "Neck", "Chest", "Pelvis" };
        ImGui::Combo( "Target Bone", &aim.TargetBone, bones, 4 );

        const char* priority[] = { "Closest to Crosshair", "Closest Distance", "Lowest HP" };
        ImGui::Combo( "Priority", &aim.Priority, priority, 3 );

        ImGui::Spacing();
        ImGui::SeparatorText( "Behavior" );

        ImGui::Checkbox( "Hold to Aim", &aim.HoldToAim );
        ImGui::Checkbox( "Sticky Target", &aim.StickyTarget );
        ImGui::Checkbox( "Visibility Check", &aim.VisCheck );

        if ( aim.StickyTarget )
            ImGui::SliderFloat( "Reacquire Delay", &aim.ReacquireDelay, 0.f, 2.f, "%.2f s" );

        ImGui::Spacing();
        ImGui::SeparatorText( "Visuals" );

        ImGui::Checkbox( "Draw FOV Circle", &aim.DrawFOV );
        if ( aim.DrawFOV )
            ImGui::ColorEdit4( "FOV Color", &aim.FOVColor.x, ImGuiColorEditFlags_NoInputs );
    }
}
