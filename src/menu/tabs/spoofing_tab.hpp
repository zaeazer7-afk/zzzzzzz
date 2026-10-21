#pragma once

#include <imgui/imgui.h>
#include <features/spoofing.hpp>
#include <menu/notifications.hpp>

namespace Menu::Tabs
{
    inline void RenderSpoofing()
    {
        auto& s = Features::Spoofing::Get();

        ImGui::SeparatorText( "Name Spoofing" );

        ImGui::Checkbox( "Spoof Name", &s.SpoofName );

        static char nameInput[64];
        static bool nameInit = false;
        if ( !nameInit )
        {
            snprintf( nameInput, sizeof( nameInput ), "%s", s.FakeName.c_str() );
            nameInit = true;
        }
        ImGui::InputText( "Fake Name", nameInput, sizeof( nameInput ) );
        ImGui::SameLine();
        if ( ImGui::Button( "Apply Name" ) && nameInput[0] )
        {
            s.SetFakeName( nameInput );
            Menu::g_Notifs.Add( std::string( "Name set to: " ) + nameInput, Menu::NotifType::Success );
        }

        ImGui::Spacing();
        ImGui::SeparatorText( "Model Spoofing" );

        ImGui::Checkbox( "Spoof Model", &s.SpoofModel );

        static char modelInput[64];
        static bool modelInit = false;
        if ( !modelInit )
        {
            snprintf( modelInput, sizeof( modelInput ), "%s", s.FakeModel.c_str() );
            modelInit = true;
        }
        ImGui::InputText( "Model Name", modelInput, sizeof( modelInput ) );

        if ( ImGui::Button( "Set Model", ImVec2( 140, 0 ) ) && modelInput[0] )
        {
            s.FakeModel = modelInput;
            s.SetModel( modelInput );
            Menu::g_Notifs.Add( std::string( "Model: " ) + modelInput, Menu::NotifType::Success );
        }
        ImGui::SameLine();
        if ( ImGui::Button( "Reset Model", ImVec2( 140, 0 ) ) )
        {
            s.SpoofModel = false;
            s.ResetModel();
            Menu::g_Notifs.Add( "Model reset to default", Menu::NotifType::Info );
        }

        ImGui::Spacing();
        ImGui::SeparatorText( "Voice" );

        ImGui::Checkbox( "Override Voice Proximity", &s.VoiceProximity );
        if ( s.VoiceProximity )
        {
            ImGui::SliderFloat( "Voice Range", &s.FakeVoiceRange, 0.f, 10000.f, "%.0f" );
            if ( ImGui::Button( "Apply Voice Range" ) )
                s.SpoofVoiceProximity();
        }
    }
}
