#pragma once

#include <imgui.h>
#include <features/combat.hpp>

namespace Menu::Tabs
{
    inline void RenderCombat()
    {
        auto& c = Features::Combat::Get();

        ImGui::SeparatorText( "Triggerbot" );
        ImGui::Checkbox( "Triggerbot", &c.Triggerbot );
        if ( c.Triggerbot )
            ImGui::SliderInt( "Delay (ms)", &c.TriggerbotDelay, 0, 500 );

        ImGui::Spacing();
        ImGui::SeparatorText( "Weapon Enhancements" );

        ImGui::Checkbox( "Rapid Fire", &c.RapidFire );
        ImGui::Checkbox( "Explosive Ammo", &c.ExplosiveAmmo );
        ImGui::Checkbox( "Infinite Ammo", &c.InfiniteAmmo );
        ImGui::Checkbox( "No Recoil", &c.NoRecoil );
        ImGui::Checkbox( "No Spread", &c.NoSpread );
        ImGui::Checkbox( "Fast Reload", &c.FastReload );

        ImGui::Spacing();
        ImGui::SeparatorText( "Damage" );

        ImGui::Checkbox( "Damage Multiplier", &c.DamageMultiplier );
        if ( c.DamageMultiplier )
            ImGui::SliderFloat( "Multiplier", &c.DamageMult, 1.f, 100.f, "%.1fx" );

        ImGui::Checkbox( "Kill Aura", &c.KillAura );
        if ( c.KillAura )
            ImGui::SliderFloat( "Aura Range", &c.KillAuraRange, 5.f, 100.f, "%.0f m" );
    }
}
