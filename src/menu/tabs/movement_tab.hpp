#pragma once

#include <imgui.h>
#include <features/movement.hpp>

namespace Menu::Tabs
{
    inline void RenderMovement()
    {
        auto& m = Features::Movement::Get();

        ImGui::SeparatorText( "Speed" );

        ImGui::Checkbox( "Super Run", &m.SuperRun );
        if ( m.SuperRun )
            ImGui::SliderFloat( "Run Speed", &m.RunSpeed, 1.f, 10.f, "%.1fx" );

        ImGui::Checkbox( "Super Jump", &m.SuperJump );
        ImGui::Checkbox( "Swim Fast", &m.SwimFast );

        ImGui::Spacing();
        ImGui::SeparatorText( "Stamina & Physics" );

        ImGui::Checkbox( "Infinite Stamina", &m.InfiniteStamina );
        ImGui::Checkbox( "No Ragdoll", &m.NoRagdoll );
        ImGui::Checkbox( "Seatbelt", &m.Seatbelt );

        ImGui::Spacing();
        ImGui::SeparatorText( "Flight" );

        ImGui::Checkbox( "Noclip", &m.Noclip );
        if ( m.Noclip )
            ImGui::SliderFloat( "Noclip Speed", &m.NoclipSpeed, 0.5f, 20.f, "%.1f" );

        ImGui::Checkbox( "Fly Hack", &m.FlyHack );
        if ( m.FlyHack )
            ImGui::SliderFloat( "Fly Speed", &m.FlySpeed, 0.5f, 20.f, "%.1f" );
    }
}
