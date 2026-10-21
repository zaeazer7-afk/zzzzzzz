#pragma once

#include <imgui/imgui.h>
#include <core/lua_runtime.hpp>
#include <core/config.hpp>
#include <menu/notifications.hpp>

#include <menu/tabs/home_tab.hpp>
#include <menu/tabs/scripts_tab.hpp>
#include <menu/tabs/executor_tab.hpp>
#include <menu/tabs/players_tab.hpp>
#include <menu/tabs/esp_tab.hpp>
#include <menu/tabs/aimbot_tab.hpp>
#include <menu/tabs/combat_tab.hpp>
#include <menu/tabs/movement_tab.hpp>
#include <menu/tabs/vehicle_tab.hpp>
#include <menu/tabs/world_tab.hpp>
#include <menu/tabs/teleport_tab.hpp>
#include <menu/tabs/troll_tab.hpp>
#include <menu/tabs/protection_tab.hpp>
#include <menu/tabs/network_tab.hpp>
#include <menu/tabs/hud_tab.hpp>
#include <menu/tabs/weapon_mods_tab.hpp>
#include <menu/tabs/ped_tab.hpp>
#include <menu/tabs/freecam_tab.hpp>
#include <menu/tabs/settings_tab.hpp>

#include <atomic>

namespace Menu
{
    class MainMenu
    {
    public:
        static MainMenu& Get()
        {
            static MainMenu inst;
            return inst;
        }

        std::atomic<bool> m_requestUnload{ false };

        bool IsOpen() const { return m_open; }
        void Toggle() { m_open = !m_open; }
        void SetOpen( bool v ) { m_open = v; }

        void Render()
        {
            g_Notifs.Render();

            if ( !m_open ) return;

            if ( !m_positioned )
            {
                auto& io = ImGui::GetIO();
                ImGui::SetNextWindowPos( ImVec2( io.DisplaySize.x * 0.5f - 450.f, io.DisplaySize.y * 0.5f - 300.f ), ImGuiCond_Once );
                ImGui::SetNextWindowSize( ImVec2( 900, 600 ), ImGuiCond_Once );
                m_positioned = true;
            }

            ImGui::Begin( "Trident Internal##main", &m_open,
                ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoScrollbar );

            DrawStatusBar();
            ImGui::Separator();

            ImGui::BeginChild( "Sidebar", ImVec2( 180, 0 ), true );
            DrawSidebar();
            ImGui::EndChild();

            ImGui::SameLine();

            ImGui::BeginChild( "Content", ImVec2( 0, 0 ), true );
            DrawContent();
            ImGui::EndChild();

            ImGui::End();
        }

    private:
        MainMenu() = default;

        bool m_open       = false;
        bool m_positioned = false;
        int  m_currentTab = 0;

        struct TabEntry
        {
            const char* name;
            void ( *render )();
        };

        static constexpr int TAB_COUNT = 19;

        TabEntry m_tabs[TAB_COUNT] = {
            { "Home",        Tabs::RenderHome },
            { "Scripts",     Tabs::RenderScripts },
            { "Executor",    Tabs::RenderExecutor },
            { "Players",     Tabs::RenderPlayers },
            { "ESP",         Tabs::RenderESP },
            { "Aimbot",      Tabs::RenderAimbot },
            { "Combat",      Tabs::RenderCombat },
            { "Movement",    Tabs::RenderMovement },
            { "Freecam",     Tabs::RenderFreecam },
            { "Vehicle",     Tabs::RenderVehicle },
            { "World",       Tabs::RenderWorld },
            { "Teleport",    Tabs::RenderTeleport },
            { "Troll",       Tabs::RenderTroll },
            { "Protection",  Tabs::RenderProtection },
            { "Network",     Tabs::RenderNetwork },
            { "HUD",         Tabs::RenderHUD },
            { "Weapon Mods", Tabs::RenderWeaponMods },
            { "Peds",        Tabs::RenderPeds },
            { "Settings",    Tabs::RenderSettings },
        };

        void DrawStatusBar()
        {
            bool luaReady = Core::LuaRuntime::Get().IsReady();
            auto& serverUrl = Core::LuaRuntime::Get().GetServerUrl();

            if ( luaReady )
                ImGui::TextColored( ImVec4( 0.f, 1.f, 0.f, 1.f ), "LUA: Ready" );
            else
                ImGui::TextColored( ImVec4( 1.f, 0.f, 0.f, 1.f ), "LUA: Not Ready" );

            ImGui::SameLine( 200.f );
            ImGui::Text( "Server: %s", serverUrl.empty() ? "Unknown" : serverUrl.c_str() );
        }

        void DrawSidebar()
        {
            for ( int i = 0; i < TAB_COUNT; ++i )
            {
                bool selected = ( m_currentTab == i );
                if ( selected )
                {
                    ImGui::PushStyleColor( ImGuiCol_Button, ImVec4( 0.35f, 0.18f, 0.55f, 1.f ) );
                    ImGui::PushStyleColor( ImGuiCol_ButtonHovered, ImVec4( 0.45f, 0.25f, 0.65f, 1.f ) );
                }

                if ( ImGui::Button( m_tabs[i].name, ImVec2( -1, 0 ) ) )
                    m_currentTab = i;

                if ( selected )
                    ImGui::PopStyleColor( 2 );
            }

            ImGui::Spacing();
            ImGui::Separator();
            ImGui::Spacing();

            ImGui::PushStyleColor( ImGuiCol_Button, ImVec4( 0.5f, 0.1f, 0.1f, 1.f ) );
            ImGui::PushStyleColor( ImGuiCol_ButtonHovered, ImVec4( 0.7f, 0.15f, 0.15f, 1.f ) );
            if ( ImGui::Button( "Unload", ImVec2( -1, 0 ) ) )
                m_requestUnload = true;
            ImGui::PopStyleColor( 2 );
        }

        void DrawContent()
        {
            if ( m_currentTab >= 0 && m_currentTab < TAB_COUNT && m_tabs[m_currentTab].render )
                m_tabs[m_currentTab].render();
        }
    };

    inline MainMenu& g_MainMenu = MainMenu::Get();
}
