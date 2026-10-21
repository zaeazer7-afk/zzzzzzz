#pragma once

#include <imgui.h>
#include <features/ped_features.hpp>

namespace Menu::Tabs
{
    inline void RenderPeds()
    {
        auto& pf = Features::PedFeatures::Get();

        ImGui::SeparatorText( "NPC Behavior" );

        ImGui::Checkbox( "Riot Mode", &pf.RiotMode );
        ImGui::Checkbox( "Freeze NPCs", &pf.FreezeNPCs );
        ImGui::Checkbox( "Everyone Attacks You", &pf.EveryoneAttacks );
        ImGui::Checkbox( "Peds Follow You", &pf.PedsFollow );

        ImGui::Spacing();
        ImGui::SeparatorText( "Density" );

        ImGui::Checkbox( "Override Traffic Density", &pf.TrafficDensityOverride );
        if ( pf.TrafficDensityOverride )
            ImGui::SliderFloat( "Traffic", &pf.TrafficDensity, 0.f, 1.f, "%.2f" );

        ImGui::Checkbox( "Override Ped Density", &pf.PedDensityOverride );
        if ( pf.PedDensityOverride )
            ImGui::SliderFloat( "Peds", &pf.PedDensity, 0.f, 1.f, "%.2f" );

        ImGui::Spacing();
        ImGui::SeparatorText( "Spawn" );

        ImGui::InputText( "Ped Model", pf.SpawnModelName, sizeof( pf.SpawnModelName ) );
        ImGui::SameLine();
        if ( ImGui::Button( "Spawn Ped" ) )
            pf.SpawnPed();

        ImGui::Combo( "Animal", &pf.SelectedAnimal, pf.AnimalModels, 17 );
        ImGui::SameLine();
        if ( ImGui::Button( "Spawn Animal" ) )
            pf.SpawnAnimal();

        ImGui::Spacing();
        ImGui::SeparatorText( "Actions" );

        if ( ImGui::Button( "Clear Nearby Peds", ImVec2( 180, 0 ) ) )
            pf.ClearNearbyPeds();

        ImGui::InputText( "Player Model", pf.PlayerModelName, sizeof( pf.PlayerModelName ) );
        ImGui::SameLine();
        if ( ImGui::Button( "Change Model" ) )
            pf.ChangePlayerModel();
    }
}
