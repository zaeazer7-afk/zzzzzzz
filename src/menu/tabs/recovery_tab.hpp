#pragma once

#include <imgui.h>
#include <features/recovery.hpp>
#include <menu/notifications.hpp>

namespace Menu::Tabs
{
    inline void RenderRecovery()
    {
        auto& r = Features::Recovery::Get();

        ImGui::SeparatorText( "Framework" );

        const char* frameworks[] = { "Auto (Try All)", "QBCore", "ESX", "vRP", "QBox", "ND" };
        ImGui::Combo( "Framework", &r.ActiveFramework, frameworks, 6 );
        ImGui::SameLine();
        if ( ImGui::Button( "Detect" ) )
        {
            r.DetectFramework();
            Menu::g_Notifs.Add( "Detecting framework...", Menu::NotifType::Info );
        }

        ImGui::Spacing();
        ImGui::SeparatorText( "Money" );

        ImGui::InputInt( "Amount", &r.MoneyAmount, 10000, 100000 );
        if ( r.MoneyAmount < 0 ) r.MoneyAmount = 0;

        if ( ImGui::Button( "Give Cash", ImVec2( 140, 0 ) ) )
        {
            r.GiveMoney();
            Menu::g_Notifs.Add( "Cash sent", Menu::NotifType::Success );
        }
        ImGui::SameLine();
        if ( ImGui::Button( "Give Dirty Money", ImVec2( 160, 0 ) ) )
        {
            r.GiveBlackMoney();
            Menu::g_Notifs.Add( "Dirty money sent", Menu::NotifType::Success );
        }

        ImGui::Spacing();
        ImGui::SeparatorText( "Items" );

        static char itemName[64] = "phone";
        static int  itemCount = 1;

        ImGui::InputText( "Item Name", itemName, sizeof( itemName ) );
        ImGui::InputInt( "Count", &itemCount, 1, 10 );
        if ( itemCount < 1 ) itemCount = 1;

        if ( ImGui::Button( "Give Item", ImVec2( 140, 0 ) ) && itemName[0] )
        {
            r.GiveItem( itemName, itemCount );
            Menu::g_Notifs.Add( std::string( "Item: " ) + itemName + " x" + std::to_string( itemCount ), Menu::NotifType::Success );
        }

        ImGui::Spacing();
        ImGui::SeparatorText( "Weapons" );

        static char weaponName[64] = "weapon_pistol";
        ImGui::InputText( "Weapon Item", weaponName, sizeof( weaponName ) );
        ImGui::SameLine();
        if ( ImGui::Button( "Give##weapon", ImVec2( 80, 0 ) ) && weaponName[0] )
        {
            r.GiveWeaponItem( weaponName );
            Menu::g_Notifs.Add( std::string( "Weapon: " ) + weaponName, Menu::NotifType::Success );
        }

        ImGui::Spacing();
        ImGui::SeparatorText( "Vehicle" );

        static char carModel[64] = "adder";
        ImGui::InputText( "Vehicle Model", carModel, sizeof( carModel ) );
        ImGui::SameLine();
        if ( ImGui::Button( "Give Car", ImVec2( 100, 0 ) ) && carModel[0] )
        {
            r.GiveCar( carModel );
            Menu::g_Notifs.Add( std::string( "Vehicle: " ) + carModel, Menu::NotifType::Success );
        }

        ImGui::Spacing();
        ImGui::SeparatorText( "Job" );

        static char jobName[64] = "police";
        static int  jobGrade = 0;
        ImGui::InputText( "Job Name", jobName, sizeof( jobName ) );
        ImGui::InputInt( "Grade", &jobGrade, 1, 1 );
        if ( jobGrade < 0 ) jobGrade = 0;
        if ( ImGui::Button( "Set Job", ImVec2( 140, 0 ) ) && jobName[0] )
        {
            r.SetJob( jobName, jobGrade );
            Menu::g_Notifs.Add( std::string( "Job: " ) + jobName + " grade " + std::to_string( jobGrade ), Menu::NotifType::Success );
        }

        ImGui::Spacing();
        ImGui::SeparatorText( "Actions" );

        if ( ImGui::Button( "Revive Self", ImVec2( 140, 0 ) ) )
        {
            r.ReviveMe();
            Menu::g_Notifs.Add( "Revive sent", Menu::NotifType::Success );
        }
        ImGui::SameLine();
        if ( ImGui::Button( "Max Stats", ImVec2( 140, 0 ) ) )
        {
            r.MaxStats();
            Menu::g_Notifs.Add( "Stats maxed", Menu::NotifType::Success );
        }
    }
}
