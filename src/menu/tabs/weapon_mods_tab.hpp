#pragma once

#include <imgui/imgui.h>
#include <features/weapon_mods.hpp>

namespace Menu::Tabs
{
    inline void RenderWeaponMods()
    {
        auto& wm = Features::WeaponMods::Get();

        ImGui::SeparatorText( "Special Guns" );

        ImGui::Checkbox( "Gravity Gun", &wm.GravityGun );
        if ( wm.GravityGun )
        {
            ImGui::SliderFloat( "Gravity Force", &wm.GravityForce, 5.f, 100.f, "%.0f" );
            ImGui::TextDisabled( "  Aim + LMB = pull, Aim + RMB = push" );
        }

        ImGui::Checkbox( "Delete Gun", &wm.DeleteGun );
        if ( wm.DeleteGun )
            ImGui::TextDisabled( "  Shoot entities to delete them" );

        ImGui::Checkbox( "Repair Gun", &wm.RepairGun );
        if ( wm.RepairGun )
            ImGui::TextDisabled( "  Shoot vehicles to fully repair them" );

        ImGui::Checkbox( "Teleport Gun", &wm.TeleportGun );
        if ( wm.TeleportGun )
            ImGui::TextDisabled( "  Shoot to teleport to impact point" );

        ImGui::Checkbox( "Money Gun", &wm.MoneyGun );
        if ( wm.MoneyGun )
            ImGui::TextDisabled( "  Shoot to spawn money pickups at target" );
    }
}
