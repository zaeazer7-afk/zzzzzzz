#pragma once

#include <imgui/imgui.h>
#include <features/world.hpp>

namespace Menu::Tabs
{
    inline void RenderWorld()
    {
        auto& w = Features::WorldFeatures::Get();

        ImGui::SeparatorText( "Self Protection" );

        ImGui::Checkbox( "God Mode", &w.GodMode );
        ImGui::SameLine( 200.f );
        ImGui::Checkbox( "Semi-God", &w.SemiGodMode );

        ImGui::Checkbox( "Invisible", &w.Invisible );
        ImGui::SameLine( 200.f );
        ImGui::Checkbox( "Auto Heal", &w.AutoHeal );

        ImGui::Checkbox( "Auto Revive", &w.AutoRevive );
        ImGui::SameLine( 200.f );
        ImGui::Checkbox( "Never Wanted", &w.NeverWanted );

        ImGui::Spacing();
        ImGui::SeparatorText( "Wanted Level" );

        ImGui::Checkbox( "Lock Wanted Level", &w.WantedLevelLock );
        if ( w.WantedLevelLock )
            ImGui::SliderInt( "Level", &w.WantedLevel, 0, 5 );

        ImGui::Spacing();
        ImGui::SeparatorText( "Vision" );

        ImGui::Checkbox( "Night Vision", &w.NightVision );
        ImGui::SameLine( 200.f );
        ImGui::Checkbox( "Thermal Vision", &w.ThermalVision );

        ImGui::Checkbox( "No Idle Camera", &w.NoIdleCam );
        ImGui::SameLine( 200.f );
        ImGui::Checkbox( "Blackout", &w.Blackout );

        ImGui::Spacing();
        ImGui::SeparatorText( "Time & Weather" );

        ImGui::SliderInt( "Hour", &w.TimeHour, 0, 23 );
        ImGui::SliderInt( "Minute", &w.TimeMinute, 0, 59 );
        if ( ImGui::Button( "Set Time" ) )
            w.SetTime( w.TimeHour, w.TimeMinute );

        ImGui::Spacing();

        ImGui::Combo( "Weather", &w.WeatherIndex, w.WeatherTypes, w.WeatherCount );
        ImGui::SameLine();
        if ( ImGui::Button( "Set Weather" ) )
            w.SetWeather( w.WeatherTypes[w.WeatherIndex] );

        ImGui::Spacing();
        ImGui::SeparatorText( "Actions" );

        if ( ImGui::Button( "Heal Self", ImVec2( 140, 0 ) ) )
            w.HealPlayer();
        ImGui::SameLine();
        if ( ImGui::Button( "Max Armor", ImVec2( 140, 0 ) ) )
            w.MaxArmor();

        if ( ImGui::Button( "Give All Weapons", ImVec2( 140, 0 ) ) )
            w.GiveAllWeapons();
        ImGui::SameLine();
        if ( ImGui::Button( "Remove All Weapons", ImVec2( 140, 0 ) ) )
            w.RemoveAllWeapons();

        if ( ImGui::Button( "Spawn Bodyguard", ImVec2( 140, 0 ) ) )
            w.SpawnBodyguard();
        ImGui::SameLine();
        if ( ImGui::Button( "Clear Area", ImVec2( 140, 0 ) ) )
            w.ClearArea();
    }
}
