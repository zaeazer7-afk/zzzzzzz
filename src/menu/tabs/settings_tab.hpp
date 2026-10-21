#pragma once

#include <imgui/imgui.h>
#include <core/config.hpp>
#include <core/lua_runtime.hpp>
#include <menu/notifications.hpp>

namespace Menu::Tabs
{
    inline void ApplyTheme( int themeIdx )
    {
        auto& style = ImGui::GetStyle();
        auto* c = style.Colors;

        switch ( themeIdx )
        {
        case 0: // Dark Purple
            c[ImGuiCol_WindowBg]        = ImVec4( 0.06f, 0.05f, 0.07f, 0.95f );
            c[ImGuiCol_ChildBg]         = ImVec4( 0.07f, 0.06f, 0.08f, 0.90f );
            c[ImGuiCol_FrameBg]         = ImVec4( 0.12f, 0.10f, 0.15f, 1.f );
            c[ImGuiCol_FrameBgHovered]  = ImVec4( 0.20f, 0.15f, 0.28f, 1.f );
            c[ImGuiCol_FrameBgActive]   = ImVec4( 0.28f, 0.18f, 0.40f, 1.f );
            c[ImGuiCol_TitleBg]         = ImVec4( 0.08f, 0.05f, 0.12f, 1.f );
            c[ImGuiCol_TitleBgActive]   = ImVec4( 0.15f, 0.08f, 0.22f, 1.f );
            c[ImGuiCol_Button]          = ImVec4( 0.20f, 0.12f, 0.30f, 1.f );
            c[ImGuiCol_ButtonHovered]   = ImVec4( 0.30f, 0.18f, 0.45f, 1.f );
            c[ImGuiCol_ButtonActive]    = ImVec4( 0.40f, 0.22f, 0.60f, 1.f );
            c[ImGuiCol_Header]          = ImVec4( 0.20f, 0.12f, 0.30f, 1.f );
            c[ImGuiCol_HeaderHovered]   = ImVec4( 0.30f, 0.18f, 0.45f, 1.f );
            c[ImGuiCol_HeaderActive]    = ImVec4( 0.40f, 0.22f, 0.60f, 1.f );
            c[ImGuiCol_CheckMark]       = ImVec4( 0.60f, 0.35f, 1.f, 1.f );
            c[ImGuiCol_SliderGrab]      = ImVec4( 0.50f, 0.30f, 0.80f, 1.f );
            c[ImGuiCol_SliderGrabActive]= ImVec4( 0.65f, 0.40f, 1.f, 1.f );
            break;
        case 1: // Dark Blue
            c[ImGuiCol_WindowBg]        = ImVec4( 0.05f, 0.06f, 0.09f, 0.95f );
            c[ImGuiCol_Button]          = ImVec4( 0.10f, 0.15f, 0.35f, 1.f );
            c[ImGuiCol_ButtonHovered]   = ImVec4( 0.15f, 0.22f, 0.50f, 1.f );
            c[ImGuiCol_ButtonActive]    = ImVec4( 0.20f, 0.30f, 0.65f, 1.f );
            c[ImGuiCol_CheckMark]       = ImVec4( 0.3f, 0.5f, 1.f, 1.f );
            c[ImGuiCol_SliderGrab]      = ImVec4( 0.2f, 0.4f, 0.9f, 1.f );
            break;
        case 2: // Dark Red
            c[ImGuiCol_WindowBg]        = ImVec4( 0.08f, 0.05f, 0.05f, 0.95f );
            c[ImGuiCol_Button]          = ImVec4( 0.35f, 0.10f, 0.10f, 1.f );
            c[ImGuiCol_ButtonHovered]   = ImVec4( 0.50f, 0.15f, 0.15f, 1.f );
            c[ImGuiCol_ButtonActive]    = ImVec4( 0.65f, 0.20f, 0.20f, 1.f );
            c[ImGuiCol_CheckMark]       = ImVec4( 1.f, 0.3f, 0.3f, 1.f );
            c[ImGuiCol_SliderGrab]      = ImVec4( 0.9f, 0.2f, 0.2f, 1.f );
            break;
        case 3: // Classic
            ImGui::StyleColorsDark();
            break;
        }
    }

    inline void RenderSettings()
    {
        ImGui::SeparatorText( "General" );

        ImGui::Text( "Menu Toggle Key: INSERT" );

        static int themeIdx = 0;
        const char* themes[] = { "Dark Purple", "Dark Blue", "Dark Red", "Classic" };
        if ( ImGui::Combo( "Theme", &themeIdx, themes, 4 ) )
            ApplyTheme( themeIdx );

        static bool autoInit = true;
        ImGui::Checkbox( "Auto-Init on Inject", &autoInit );

        ImGui::Spacing();
        ImGui::SeparatorText( "Config" );

        if ( ImGui::Button( "Save Config", ImVec2( 140, 0 ) ) )
        {
            Core::g_Config.SyncToFile();
            Menu::g_Notifs.Add( "Config saved", Menu::NotifType::Success );
        }
        ImGui::SameLine();
        if ( ImGui::Button( "Load Config", ImVec2( 140, 0 ) ) )
        {
            Core::g_Config.Load();
            Menu::g_Notifs.Add( "Config loaded", Menu::NotifType::Success );
        }

        ImGui::Spacing();
        ImGui::SeparatorText( "Lua" );

        if ( ImGui::Button( "Re-scan Lua VM" ) )
        {
            Core::LuaRuntime::Get().Rescan();
            Menu::g_Notifs.Add( "Lua VM re-scanned", Menu::NotifType::Info );
        }

        bool luaReady = Core::LuaRuntime::Get().IsReady();
        ImGui::Text( "Lua Status: %s", luaReady ? "Ready" : "Not Ready" );

        ImGui::Spacing();
        ImGui::SeparatorText( "About" );

        ImGui::Text( "Trident Internal v2.0" );
        ImGui::Text( "Build: " __DATE__ " " __TIME__ );
        ImGui::TextDisabled( "FiveM Internal Cheat" );
    }
}
