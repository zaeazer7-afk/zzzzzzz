#pragma once

#include <imgui/imgui.h>
#include <core/scheduler.hpp>
#include <string>
#include <chrono>
#include <deque>

namespace Features
{
    enum class CrosshairStyle : int
    {
        None = 0,
        Cross,
        Circle,
        Dot,
        CrossDot
    };

    class HUD
    {
    public:
        static HUD& Get()
        {
            static HUD inst;
            return inst;
        }

        bool  Watermark       = true;
        bool  FPSCounter      = true;
        bool  CoordDisplay     = false;
        bool  SpeedDisplay     = false;
        bool  CrosshairEnabled = false;
        int   CrosshairStyle   = static_cast<int>( CrosshairStyle::Cross );
        float CrosshairSize    = 6.f;
        ImVec4 CrosshairColor  = ImVec4( 0.f, 1.f, 0.f, 1.f );
        bool  SessionInfo      = false;
        bool  MinimapEnlarge   = false;

        float m_currentFps  = 0.f;
        float m_playerX = 0, m_playerY = 0, m_playerZ = 0;
        float m_speed       = 0.f;
        std::string m_serverName;
        int   m_playerCount = 0;

        void Update()
        {
            TrackFPS();
            FetchPlayerData();

            auto* drawList = ImGui::GetBackgroundDrawList();
            auto  display  = ImGui::GetIO().DisplaySize;

            if ( Watermark )
                DrawWatermark( drawList, display );

            if ( FPSCounter )
                DrawFPS( drawList, display );

            if ( CoordDisplay )
                DrawCoords( drawList, display );

            if ( SpeedDisplay )
                DrawSpeed( drawList, display );

            if ( CrosshairEnabled )
                DrawCrosshair( drawList, display );

            if ( SessionInfo )
                DrawSessionInfo( drawList, display );

            if ( MinimapEnlarge )
            {
                Core::g_Scheduler.Execute( R"(
                    SetRadarBigmapEnabled(true, false)
                )" );
            }
        }

    private:
        HUD() = default;

        std::deque<float> m_frameTimes;
        std::chrono::steady_clock::time_point m_lastFrame{};
        std::chrono::steady_clock::time_point m_lastDataFetch{};

        void TrackFPS()
        {
            auto now = std::chrono::steady_clock::now();
            float dt = std::chrono::duration<float>( now - m_lastFrame ).count();
            m_lastFrame = now;

            if ( dt > 0.f && dt < 1.f )
            {
                m_frameTimes.push_back( dt );
                if ( m_frameTimes.size() > 60 )
                    m_frameTimes.pop_front();
            }

            float sum = 0;
            for ( auto& t : m_frameTimes ) sum += t;
            if ( !m_frameTimes.empty() )
                m_currentFps = static_cast<float>( m_frameTimes.size() ) / sum;
        }

        void FetchPlayerData()
        {
            auto now = std::chrono::steady_clock::now();
            if ( std::chrono::duration<float>( now - m_lastDataFetch ).count() < 0.2f )
                return;
            m_lastDataFetch = now;

            auto result = Core::g_Scheduler.Execute( R"(
                local ped = PlayerPedId()
                local c = GetEntityCoords(ped)
                local spd = GetEntitySpeed(ped)
                return string.format('%.2f|%.2f|%.2f|%.2f', c.x, c.y, c.z, spd)
            )" );

            if ( result.Ok && !result.Output.empty() )
            {
                float vals[4] = {};
                int idx = 0;
                std::string data = result.Output;
                size_t pos;
                while ( idx < 4 && ( pos = data.find( '|' ) ) != std::string::npos )
                {
                    vals[idx++] = std::stof( data.substr( 0, pos ) );
                    data.erase( 0, pos + 1 );
                }
                if ( idx < 4 && !data.empty() )
                    vals[idx] = std::stof( data );

                m_playerX = vals[0];
                m_playerY = vals[1];
                m_playerZ = vals[2];
                m_speed   = vals[3] * 3.6f;
            }
        }

        void DrawWatermark( ImDrawList* dl, const ImVec2& display )
        {
            const char* text = "Trident Internal";
            ImVec2 textSize = ImGui::CalcTextSize( text );
            ImVec2 pos( 10, 10 );
            ImVec2 padding( 8, 4 );

            dl->AddRectFilled(
                ImVec2( pos.x - padding.x, pos.y - padding.y ),
                ImVec2( pos.x + textSize.x + padding.x, pos.y + textSize.y + padding.y ),
                IM_COL32( 15, 15, 15, 200 ), 4.f );
            dl->AddRect(
                ImVec2( pos.x - padding.x, pos.y - padding.y ),
                ImVec2( pos.x + textSize.x + padding.x, pos.y + textSize.y + padding.y ),
                IM_COL32( 100, 60, 180, 255 ), 4.f );
            dl->AddText( pos, IM_COL32( 100, 60, 180, 255 ), text );
        }

        void DrawFPS( ImDrawList* dl, const ImVec2& display )
        {
            char buf[32];
            snprintf( buf, sizeof( buf ), "%.0f FPS", m_currentFps );
            ImVec2 textSize = ImGui::CalcTextSize( buf );
            ImVec2 pos( display.x - textSize.x - 10, 10 );

            dl->AddRectFilled(
                ImVec2( pos.x - 6, pos.y - 3 ),
                ImVec2( pos.x + textSize.x + 6, pos.y + textSize.y + 3 ),
                IM_COL32( 15, 15, 15, 200 ), 4.f );

            ImU32 col = m_currentFps >= 60 ? IM_COL32( 0, 255, 0, 255 ) :
                        m_currentFps >= 30 ? IM_COL32( 255, 255, 0, 255 ) :
                                             IM_COL32( 255, 0, 0, 255 );
            dl->AddText( pos, col, buf );
        }

        void DrawCoords( ImDrawList* dl, const ImVec2& display )
        {
            char buf[128];
            snprintf( buf, sizeof( buf ), "X: %.2f  Y: %.2f  Z: %.2f", m_playerX, m_playerY, m_playerZ );
            ImVec2 textSize = ImGui::CalcTextSize( buf );
            ImVec2 pos( 10, display.y - textSize.y - 30 );

            dl->AddRectFilled(
                ImVec2( pos.x - 6, pos.y - 3 ),
                ImVec2( pos.x + textSize.x + 6, pos.y + textSize.y + 3 ),
                IM_COL32( 15, 15, 15, 180 ), 4.f );
            dl->AddText( pos, IM_COL32( 200, 200, 200, 255 ), buf );
        }

        void DrawSpeed( ImDrawList* dl, const ImVec2& display )
        {
            char buf[64];
            snprintf( buf, sizeof( buf ), "%.0f km/h", m_speed );
            ImVec2 textSize = ImGui::CalcTextSize( buf );
            ImVec2 pos( 10, display.y - textSize.y - 10 );

            dl->AddRectFilled(
                ImVec2( pos.x - 6, pos.y - 3 ),
                ImVec2( pos.x + textSize.x + 6, pos.y + textSize.y + 3 ),
                IM_COL32( 15, 15, 15, 180 ), 4.f );
            dl->AddText( pos, IM_COL32( 200, 200, 200, 255 ), buf );
        }

        void DrawCrosshair( ImDrawList* dl, const ImVec2& display )
        {
            ImVec2 center( display.x * 0.5f, display.y * 0.5f );
            ImU32 col = ImGui::ColorConvertFloat4ToU32( CrosshairColor );
            float s = CrosshairSize;

            auto style = static_cast<Features::CrosshairStyle>( CrosshairStyle );

            if ( style == Features::CrosshairStyle::Cross || style == Features::CrosshairStyle::CrossDot )
            {
                dl->AddLine( ImVec2( center.x - s, center.y ), ImVec2( center.x + s, center.y ), col, 1.5f );
                dl->AddLine( ImVec2( center.x, center.y - s ), ImVec2( center.x, center.y + s ), col, 1.5f );
            }

            if ( style == Features::CrosshairStyle::Circle )
                dl->AddCircle( center, s, col, 24, 1.5f );

            if ( style == Features::CrosshairStyle::Dot || style == Features::CrosshairStyle::CrossDot )
                dl->AddCircleFilled( center, 2.f, col );
        }

        void DrawSessionInfo( ImDrawList* dl, const ImVec2& display )
        {
            if ( m_serverName.empty() ) return;
            char buf[256];
            snprintf( buf, sizeof( buf ), "%s (%d players)", m_serverName.c_str(), m_playerCount );
            ImVec2 textSize = ImGui::CalcTextSize( buf );
            ImVec2 pos( display.x - textSize.x - 10, display.y - textSize.y - 10 );

            dl->AddRectFilled(
                ImVec2( pos.x - 6, pos.y - 3 ),
                ImVec2( pos.x + textSize.x + 6, pos.y + textSize.y + 3 ),
                IM_COL32( 15, 15, 15, 180 ), 4.f );
            dl->AddText( pos, IM_COL32( 180, 180, 180, 255 ), buf );
        }
    };

    inline HUD& g_HUD = HUD::Get();
}
