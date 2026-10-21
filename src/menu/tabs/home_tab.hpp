#pragma once

#include <imgui.h>
#include <features/esp.hpp>
#include <features/aimbot.hpp>
#include <core/lua_runtime.hpp>
#include <menu/notifications.hpp>

namespace Menu::Tabs
{
    inline void RenderHome()
    {
        ImGui::PushStyleColor( ImGuiCol_Text, ImVec4( 0.6f, 0.4f, 1.f, 1.f ) );
        ImGui::SetWindowFontScale( 1.4f );
        ImGui::Text( "Trident Internal" );
        ImGui::SetWindowFontScale( 1.f );
        ImGui::PopStyleColor();

        ImGui::TextDisabled( "v2.0 | FiveM Internal" );
        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        ImGui::SeparatorText( "Status" );
        bool luaReady = Core::LuaRuntime::Get().IsReady();
        ImGui::BulletText( "Lua VM: %s", luaReady ? "Ready" : "Not Ready" );
        ImGui::BulletText( "Server: %s", Core::LuaRuntime::Get().GetServerUrl().empty() ? "Unknown" : Core::LuaRuntime::Get().GetServerUrl().c_str() );

        ImGui::Spacing();
        ImGui::SeparatorText( "Quick Toggles" );

        auto& esp = Features::ESP::Get();
        auto& aim = Features::Aimbot::Get();

        ImGui::Checkbox( "ESP", &esp.Enabled );
        ImGui::SameLine( 200.f );
        ImGui::Checkbox( "Aimbot", &aim.Enabled );

        ImGui::Spacing();
        ImGui::SeparatorText( "Recent Notifications" );

        auto& recent = Menu::g_Notifs.Recent();
        if ( recent.empty() )
        {
            ImGui::TextDisabled( "No recent notifications." );
        }
        else
        {
            for ( int i = static_cast<int>( recent.size() ) - 1; i >= 0 && i >= static_cast<int>( recent.size() ) - 5; --i )
            {
                ImVec4 col;
                switch ( recent[i].type )
                {
                case Menu::NotifType::Info:    col = ImVec4( 0.f, 0.8f, 1.f, 1.f ); break;
                case Menu::NotifType::Success: col = ImVec4( 0.f, 0.9f, 0.3f, 1.f ); break;
                case Menu::NotifType::Warning: col = ImVec4( 1.f, 0.8f, 0.f, 1.f ); break;
                case Menu::NotifType::Error:   col = ImVec4( 1.f, 0.2f, 0.2f, 1.f ); break;
                }
                ImGui::TextColored( col, "%s", recent[i].message.c_str() );
            }
        }
    }
}
