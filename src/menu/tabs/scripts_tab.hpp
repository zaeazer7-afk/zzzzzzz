#pragma once

#include <imgui/imgui.h>
#include <scripts/library.hpp>
#include <core/scheduler.hpp>
#include <menu/notifications.hpp>
#include <string>
#include <unordered_map>

namespace Menu::Tabs
{
    inline void RenderScripts()
    {
        auto& lib = Scripts::Library::Get();

        static char searchBuf[128] = "";
        ImGui::InputText( "Search Scripts", searchBuf, sizeof( searchBuf ) );
        ImGui::Spacing();

        static std::unordered_map<std::string, std::unordered_map<std::string, char[128]>> paramBuffers;

        ImGui::BeginChild( "ScriptList", ImVec2( 0, 0 ), true );

        auto categories = lib.GetCategories();
        for ( auto& catName : categories )
        {
            auto scripts = lib.GetByCategory( catName );
            if ( scripts.empty() ) continue;

            ImGui::PushStyleColor( ImGuiCol_Text, lib.GetCategoryColor( catName ) );
            bool open = ImGui::TreeNodeEx( catName.c_str(), ImGuiTreeNodeFlags_DefaultOpen );
            ImGui::PopStyleColor();

            if ( !open ) continue;

            for ( auto* s : scripts )
            {
                std::string lowerName = s->name;
                std::string query = searchBuf;
                for ( auto& c : lowerName ) c = static_cast<char>( tolower( c ) );
                for ( auto& c : query ) c = static_cast<char>( tolower( c ) );
                if ( !query.empty() && lowerName.find( query ) == std::string::npos )
                    continue;

                std::string uid = catName + "." + s->name;
                ImGui::PushID( uid.c_str() );

                if ( ImGui::TreeNode( s->name.c_str() ) )
                {
                    if ( !s->description.empty() )
                        ImGui::TextDisabled( "%s", s->description.c_str() );

                    std::unordered_map<std::string, std::string> paramValues;
                    for ( auto& p : s->params )
                    {
                        auto& buf = paramBuffers[uid][p.name];
                        if ( buf[0] == '\0' && !p.defaultValue.empty() )
                        {
                            strncpy( buf, p.defaultValue.c_str(), sizeof( buf ) - 1 );
                            buf[sizeof( buf ) - 1] = '\0';
                        }

                        ImGui::PushItemWidth( 150 );
                        ImGui::InputText( p.name.c_str(), buf, sizeof( buf ) );
                        ImGui::PopItemWidth();
                        if ( !p.description.empty() )
                        {
                            ImGui::SameLine();
                            ImGui::TextDisabled( "(%s)", p.description.c_str() );
                        }
                        paramValues[p.name] = buf;
                    }

                    if ( ImGui::Button( "Run", ImVec2( 80, 0 ) ) )
                    {
                        std::string code = Scripts::Library::BuildCode( *s, paramValues );
                        auto result = Core::g_Scheduler.ExecuteSafe( code.c_str() );
                        if ( result.Ok )
                            Menu::g_Notifs.Add( s->name + " executed", Menu::NotifType::Success );
                        else
                            Menu::g_Notifs.Add( s->name + " failed: " + result.Error, Menu::NotifType::Error );
                    }

                    ImGui::TreePop();
                }

                ImGui::PopID();
            }

            ImGui::TreePop();
        }

        ImGui::EndChild();
    }
}
