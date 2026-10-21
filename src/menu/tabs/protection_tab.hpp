#pragma once

#include <imgui/imgui.h>
#include <features/protection.hpp>

namespace Menu::Tabs
{
    inline void RenderProtection()
    {
        auto& p = Features::Protection::Get();

        ImGui::SeparatorText( "Anti-Features" );

        ImGui::Checkbox( "Anti-Explosion", &p.AntiExplosion );
        if ( p.AntiExplosion )
            ImGui::TextDisabled( "  Dodges explosions near you" );

        ImGui::Checkbox( "Anti-Teleport", &p.AntiTeleport );
        if ( p.AntiTeleport )
            ImGui::TextDisabled( "  Reverts unexpected position changes > 50m" );

        ImGui::Checkbox( "Anti-Freeze", &p.AntiFreeze );
        if ( p.AntiFreeze )
            ImGui::TextDisabled( "  Unfreezes position if frozen by others" );

        ImGui::Checkbox( "Anti-Weapon Remove", &p.AntiWeaponRemove );
        if ( p.AntiWeaponRemove )
            ImGui::TextDisabled( "  Restores weapons if removed by server" );

        ImGui::Checkbox( "Anti-Ragdoll", &p.AntiRagdoll );
        ImGui::Checkbox( "Anti-Fall", &p.AntiFall );
        if ( p.AntiFall )
            ImGui::TextDisabled( "  Auto-parachute on high falls, rescue from void" );

        ImGui::Checkbox( "Anti-Cage", &p.AntiCage );
        if ( p.AntiCage )
            ImGui::TextDisabled( "  Deletes nearby props that could be cages" );

        ImGui::Checkbox( "Anti-Attach", &p.AntiAttach );
        if ( p.AntiAttach )
            ImGui::TextDisabled( "  Removes objects attached to your ped" );

        ImGui::Checkbox( "Anti-Ptfx", &p.AntiPtfx );
        if ( p.AntiPtfx )
            ImGui::TextDisabled( "  Removes particle effects in range" );

        ImGui::Checkbox( "Anti-Invisible", &p.AntiInvisible );
        if ( p.AntiInvisible )
            ImGui::TextDisabled( "  Forces self visible if someone hides you" );

        ImGui::Spacing();
        ImGui::SeparatorText( "Cleanup" );

        if ( ImGui::Button( "Delete Nearby Objects", ImVec2( 200, 0 ) ) )
            p.DeleteNearbyObjects();
        ImGui::SameLine();
        if ( ImGui::Button( "Delete Nearby Vehicles", ImVec2( 200, 0 ) ) )
            p.DeleteNearbyVehicles();
        if ( ImGui::Button( "Cleanup Area", ImVec2( 200, 0 ) ) )
            p.CleanupArea();
    }
}
