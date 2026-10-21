#pragma once

#include <imgui.h>
#include <features/world.hpp>

namespace Menu::Tabs
{
    inline void RenderWorld()
    {
        auto& w = Features::World::Get();

        ImGui::SeparatorText( "Self Protection" );

        ImGui::Checkbox( "God Mode", &w.GodMode );
        ImGui::SameLine( 200.f );
        ImGui::Checkbox( "Semi-God", &w.SemiGod );

        ImGui::Checkbox( "Invisible", &w.Invisible );
        ImGui::SameLine( 200.f );
        ImGui::Checkbox( "Auto Heal", &w.AutoHeal );

        ImGui::Checkbox( "Auto Revive", &w.AutoRevive );

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
            w.SetTime();

        ImGui::Spacing();

        const char* weathers[] = {
            "CLEAR", "EXTRASUNNY", "CLOUDS", "OVERCAST", "RAIN",
            "CLEARING", "THUNDER", "SMOG", "FOGGY", "XMAS",
            "SNOWLIGHT", "BLIZZARD", "NEUTRAL", "HALLOWEEN"
        };
        ImGui::Combo( "Weather", &w.SelectedWeather, weathers, 14 );
        ImGui::SameLine();
        if ( ImGui::Button( "Set Weather" ) )
            w.SetWeather();

        ImGui::Spacing();
        ImGui::SeparatorText( "Actions" );

        if ( ImGui::Button( "Heal Self", ImVec2( 140, 0 ) ) )
            w.HealSelf();
        ImGui::SameLine();
        if ( ImGui::Button( "Give All Weapons", ImVec2( 140, 0 ) ) )
            w.GiveAllWeapons();

        if ( ImGui::Button( "Remove All Weapons", ImVec2( 140, 0 ) ) )
            w.RemoveAllWeapons();
        ImGui::SameLine();
        if ( ImGui::Button( "Spawn Bodyguard", ImVec2( 140, 0 ) ) )
            w.SpawnBodyguard();
    }
}
