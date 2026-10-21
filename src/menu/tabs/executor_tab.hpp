#pragma once

#include <imgui.h>
#include <core/scheduler.hpp>
#include <menu/notifications.hpp>
#include <string>

namespace Menu::Tabs
{
    inline void RenderExecutor()
    {
        static int subTab = 0;
        const char* subTabs[] = { "Executor", "Events", "History" };
        for ( int i = 0; i < 3; ++i )
        {
            if ( i > 0 ) ImGui::SameLine();
            if ( ImGui::Button( subTabs[i], ImVec2( 100, 0 ) ) )
                subTab = i;
        }
        ImGui::Separator();
        ImGui::Spacing();

        if ( subTab == 0 )
        {
            static char codeBuf[4096] = "";
            static std::string output;

            ImGui::Text( "Lua Code:" );
            ImGui::InputTextMultiline( "##code", codeBuf, sizeof( codeBuf ),
                ImVec2( -1, 200 ), ImGuiInputTextFlags_AllowTabInput );

            if ( ImGui::Button( "Execute", ImVec2( 120, 0 ) ) && codeBuf[0] )
            {
                auto result = Core::g_Scheduler.Execute( codeBuf );
                output = result.Ok ? ( "OK: " + result.Output ) : ( "FAIL: " + result.Error );
            }
            ImGui::SameLine();
            if ( ImGui::Button( "Execute Unsafe", ImVec2( 120, 0 ) ) && codeBuf[0] )
            {
                auto result = Core::g_Scheduler.ExecuteSafe( codeBuf );
                output = result.Ok ? ( "OK: " + result.Output ) : ( "FAIL: " + result.Error );
            }
            ImGui::SameLine();
            if ( ImGui::Button( "Clear", ImVec2( 80, 0 ) ) )
            {
                codeBuf[0] = '\0';
                output.clear();
            }

            if ( !output.empty() )
            {
                ImGui::Spacing();
                ImGui::SeparatorText( "Output" );
                ImGui::BeginChild( "Output", ImVec2( 0, 80 ), true );
                ImGui::TextWrapped( "%s", output.c_str() );
                ImGui::EndChild();
            }
        }
        else if ( subTab == 1 )
        {
            static char eventName[256] = "";
            static char eventArgs[512] = "";
            static bool serverEvent = true;
            static char consoleCmd[256] = "";

            ImGui::SeparatorText( "Trigger Event" );

            ImGui::InputText( "Event Name", eventName, sizeof( eventName ) );
            ImGui::InputText( "Arguments (JSON)", eventArgs, sizeof( eventArgs ) );
            ImGui::Checkbox( "Server Event", &serverEvent );

            if ( ImGui::Button( "Send Event", ImVec2( 120, 0 ) ) && eventName[0] )
            {
                if ( serverEvent )
                    Core::g_Scheduler.TriggerServerEvent( eventName, eventArgs );
                else
                    Core::g_Scheduler.TriggerClientEvent( eventName, eventArgs );
                Menu::g_Notifs.Add( std::string( "Event sent: " ) + eventName, Menu::NotifType::Info );
            }

            ImGui::Spacing();
            ImGui::SeparatorText( "Console Command" );

            ImGui::InputText( "Command", consoleCmd, sizeof( consoleCmd ) );
            ImGui::SameLine();
            if ( ImGui::Button( "Execute##cmd", ImVec2( 80, 0 ) ) && consoleCmd[0] )
            {
                Core::g_Scheduler.ExecuteCommand( consoleCmd );
                Menu::g_Notifs.Add( std::string( "Command: " ) + consoleCmd, Menu::NotifType::Info );
            }
        }
        else if ( subTab == 2 )
        {
            ImGui::SeparatorText( "Execution History" );

            auto& history = Core::g_Scheduler.GetHistory();
            if ( history.empty() )
            {
                ImGui::TextDisabled( "No history yet." );
                return;
            }

            ImGui::BeginChild( "History", ImVec2( 0, 0 ), true );
            for ( int i = static_cast<int>( history.size() ) - 1; i >= 0; --i )
            {
                auto& h = history[i];
                ImVec4 col = h.Ok ? ImVec4( 0.f, 1.f, 0.f, 1.f ) : ImVec4( 1.f, 0.f, 0.f, 1.f );
                ImGui::PushID( i );
                ImGui::TextColored( col, "[%s]", h.Ok ? "OK" : "FAIL" );
                ImGui::SameLine();
                ImGui::Text( "%s", h.Label.empty() ? h.Code.substr( 0, 60 ).c_str() : h.Label.c_str() );
                if ( ImGui::IsItemHovered() && !h.Code.empty() )
                    ImGui::SetTooltip( "%s", h.Code.c_str() );
                ImGui::PopID();
            }
            ImGui::EndChild();
        }
    }
}
