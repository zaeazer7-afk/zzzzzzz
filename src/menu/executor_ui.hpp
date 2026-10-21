#pragma once

#include <imgui.h>
#include <core/scheduler.hpp>
#include <scripts/library.hpp>

#include <string>
#include <algorithm>
#include <cstring>

namespace Menu
{
    class ExecutorUI
    {
    public:

        void Render()
        {
            static int  selTab = 0;
            const char* tabs[] = { "Scripts", "Executor", "Events", "History" };

            ImGui::BeginTabBar( "##exec_tabs" );
            for ( int i = 0; i < 4; i++ )
            {
                if ( ImGui::BeginTabItem( tabs[i] ) )
                {
                    selTab = i;
                    ImGui::EndTabItem();
                }
            }
            ImGui::EndTabBar();

            ImGui::Separator();

            switch ( selTab )
            {
            case 0: RenderScripts();  break;
            case 1: RenderExecutor(); break;
            case 2: RenderEvents();   break;
            case 3: RenderHistory();  break;
            }
        }

    private:

        void RenderScripts()
        {
            static char search[64] = {};
            static char param[256] = {};
            static int  selCat = -1;

            ImGui::SetNextItemWidth( 250.f );
            ImGui::InputTextWithHint( "##script_search", "Search scripts...", search, sizeof(search) );
            ImGui::SameLine();

            if ( ImGui::SmallButton( "All" ) ) selCat = -1;
            for ( int c = 0; c < Scripts::Cat_COUNT; c++ )
            {
                ImGui::SameLine();
                bool active = ( selCat == c );
                if ( active ) ImGui::PushStyleColor( ImGuiCol_Button, ImGui::GetStyleColorVec4( ImGuiCol_ButtonActive ) );
                if ( ImGui::SmallButton( Scripts::g_CategoryNames[c] ) )
                    selCat = ( selCat == c ) ? -1 : c;
                if ( active ) ImGui::PopStyleColor();
            }

            ImGui::Separator();

            std::string qLow( search );
            std::transform( qLow.begin(), qLow.end(), qLow.begin(),
                []( unsigned char c ) { return (char)tolower(c); } );

            ImGui::BeginChild( "##script_list", ImVec2( 0, -1 ), false );

            for ( int i = 0; i < Scripts::g_ScriptCount; i++ )
            {
                const auto& s = Scripts::g_Library[i];
                if ( selCat >= 0 && s.Cat != selCat ) continue;
                if ( qLow.size() )
                {
                    std::string nm( s.Name );
                    std::transform( nm.begin(), nm.end(), nm.begin(),
                        []( unsigned char c ) { return (char)tolower(c); } );
                    if ( nm.find( qLow ) == std::string::npos ) continue;
                }

                ImGui::PushID( i );

                ImVec4 col;
                if ( s.Cat == Scripts::Cat_QBCore )      col = ImVec4( 0.3f, 0.7f, 1.f, 1.f );
                else if ( s.Cat == Scripts::Cat_ESX )     col = ImVec4( 1.f, 0.6f, 0.3f, 1.f );
                else if ( s.Cat == Scripts::Cat_Troll )   col = ImVec4( 1.f, 0.3f, 0.3f, 1.f );
                else if ( s.Cat == Scripts::Cat_Utility ) col = ImVec4( 0.8f, 0.8f, 0.3f, 1.f );
                else                                      col = ImVec4( 0.4f, 1.f, 0.5f, 1.f );

                ImGui::PushStyleColor( ImGuiCol_Text, col );
                bool open = ImGui::TreeNode( s.Name );
                ImGui::PopStyleColor();

                if ( ImGui::IsItemHovered() ) ImGui::SetTooltip( "%s", s.Desc );

                if ( open )
                {
                    ImGui::TextDisabled( "%s", s.Desc );
                    ImGui::TextDisabled( "[%s]", Scripts::g_CategoryNames[ s.Cat ] );

                    ImGui::PushStyleColor( ImGuiCol_Text, ImVec4( 0.6f, 0.6f, 0.6f, 1.f ) );
                    std::string preview( s.Code );
                    if ( preview.size() > 120 ) preview = preview.substr( 0, 120 ) + "...";
                    ImGui::TextWrapped( "%s", preview.c_str() );
                    ImGui::PopStyleColor();

                    if ( s.HasParam )
                    {
                        ImGui::SetNextItemWidth( 300.f );
                        ImGui::InputTextWithHint( "##param", s.ParamHint, param, sizeof(param) );
                    }

                    if ( ImGui::Button( "Run", ImVec2( 80.f, 26.f ) ) )
                    {
                        std::string code = Scripts::BuildCode( i, param );
                        auto res = Core::g_Scheduler.ExecuteSafe( code.c_str(), s.Name );
                        if ( !res.Ok && !res.Error.empty() )
                            m_lastError = res.Error;
                        else
                            m_lastError.clear();
                    }

                    if ( !m_lastError.empty() )
                    {
                        ImGui::SameLine();
                        ImGui::TextColored( ImVec4( 1, 0.3f, 0.3f, 1 ), "%s", m_lastError.c_str() );
                    }

                    ImGui::TreePop();
                }

                ImGui::PopID();
            }

            ImGui::EndChild();
        }

        void RenderExecutor()
        {
            static char codeBuf[4096] = {};

            ImGui::Text( "Lua Code:" );
            ImGui::InputTextMultiline( "##code_editor", codeBuf, sizeof(codeBuf),
                ImVec2( -1, ImGui::GetContentRegionAvail().y - 80.f ) );

            if ( ImGui::Button( "Execute", ImVec2( 120.f, 30.f ) ) )
            {
                auto res = Core::g_Scheduler.ExecuteSafe( codeBuf, "editor" );
                m_execOutput = res.Ok ? ( res.Output.empty() ? "OK" : res.Output ) : ( "Error: " + res.Error );
            }
            ImGui::SameLine();
            if ( ImGui::Button( "Execute Unsafe", ImVec2( 140.f, 30.f ) ) )
            {
                auto res = Core::g_Scheduler.Execute( codeBuf, "editor-raw" );
                m_execOutput = res.Ok ? ( res.Output.empty() ? "OK" : res.Output ) : ( "Error: " + res.Error );
            }
            ImGui::SameLine();
            if ( ImGui::Button( "Clear", ImVec2( 80.f, 30.f ) ) )
            {
                codeBuf[0] = 0;
                m_execOutput.clear();
            }

            if ( !m_execOutput.empty() )
            {
                ImVec4 col = ( m_execOutput.substr( 0, 5 ) == "Error" )
                    ? ImVec4( 1, 0.3f, 0.3f, 1 ) : ImVec4( 0.3f, 1, 0.5f, 1 );
                ImGui::TextColored( col, "%s", m_execOutput.c_str() );
            }
        }

        void RenderEvents()
        {
            static char eventName[256] = {};
            static char eventArgs[1024] = {};
            static bool isServer = true;

            ImGui::Text( "Event Name:" );
            ImGui::SetNextItemWidth( -1 );
            ImGui::InputTextWithHint( "##event_name", "QBCore:Server:SetMoney", eventName, sizeof(eventName) );

            ImGui::Text( "Lua Arguments:" );
            ImGui::SetNextItemWidth( -1 );
            ImGui::InputTextWithHint( "##event_args", "'cash', 500000", eventArgs, sizeof(eventArgs) );

            ImGui::Checkbox( "Server Event", &isServer );

            ImGui::Spacing();

            if ( ImGui::Button( "Send Event", ImVec2( 140.f, 30.f ) ) )
            {
                Core::ExecResult res;
                if ( isServer )
                    res = Core::g_Scheduler.TriggerServerEvent( eventName, eventArgs );
                else
                    res = Core::g_Scheduler.TriggerClientEvent( eventName, eventArgs );

                m_eventOutput = res.Ok ? "Sent" : ( "Error: " + res.Error );
            }

            ImGui::SameLine();

            ImGui::Text( "Console Command:" );
            static char cmdBuf[512] = {};
            ImGui::SetNextItemWidth( 300.f );
            ImGui::InputTextWithHint( "##cmd", "e.g. heal, tp, fix", cmdBuf, sizeof(cmdBuf) );
            ImGui::SameLine();
            if ( ImGui::Button( "Run Cmd", ImVec2( 100.f, 30.f ) ) )
            {
                auto res = Core::g_Scheduler.ExecuteCommand( cmdBuf );
                m_eventOutput = res.Ok ? "Sent" : ( "Error: " + res.Error );
            }

            if ( !m_eventOutput.empty() )
            {
                ImVec4 col = ( m_eventOutput.substr( 0, 5 ) == "Error" )
                    ? ImVec4( 1, 0.3f, 0.3f, 1 ) : ImVec4( 0.3f, 1, 0.5f, 1 );
                ImGui::TextColored( col, "%s", m_eventOutput.c_str() );
            }

            ImGui::Separator();
            ImGui::TextDisabled( "Events run inside the resource's Lua VM." );
            ImGui::TextDisabled( "TriggerServerEvent carries the resource identity." );
        }

        void RenderHistory()
        {
            if ( ImGui::Button( "Clear History" ) )
                Core::g_Scheduler.ClearHistory();

            ImGui::Separator();

            auto hist = Core::g_Scheduler.GetHistory();
            ImGui::BeginChild( "##hist", ImVec2( 0, -1 ), false );

            for ( int i = (int)hist.size() - 1; i >= 0; i-- )
            {
                const auto& h = hist[i];
                ImVec4 col = h.Ok ? ImVec4( 0.3f, 1, 0.5f, 1 ) : ImVec4( 1, 0.3f, 0.3f, 1 );

                ImGui::PushStyleColor( ImGuiCol_Text, col );
                ImGui::Text( "%s", h.Ok ? "[OK]" : "[FAIL]" );
                ImGui::PopStyleColor();

                ImGui::SameLine();
                ImGui::Text( "[%s]", h.Label.c_str() );

                if ( !h.Error.empty() )
                {
                    ImGui::SameLine();
                    ImGui::TextColored( ImVec4( 1, 0.5f, 0.3f, 1 ), "%s", h.Error.c_str() );
                }

                if ( ImGui::IsItemHovered() && !h.Code.empty() )
                    ImGui::SetTooltip( "%s", h.Code.c_str() );
            }

            ImGui::EndChild();
        }

        std::string m_lastError;
        std::string m_execOutput;
        std::string m_eventOutput;
    };
}
