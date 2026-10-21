#pragma once

#include <imgui.h>
#include <string>
#include <vector>
#include <chrono>
#include <algorithm>

namespace Menu
{
    enum class NotifType { Info, Success, Warning, Error };

    struct Notification
    {
        std::string message;
        NotifType   type;
        std::chrono::steady_clock::time_point created;
        float       lifetime = 3.f;
    };

    class Notifications
    {
    public:
        static Notifications& Get()
        {
            static Notifications inst;
            return inst;
        }

        void Add( const std::string& msg, NotifType type = NotifType::Info )
        {
            m_notifs.push_back( { msg, type, std::chrono::steady_clock::now(), 3.f } );
            if ( m_notifs.size() > 5 )
                m_notifs.erase( m_notifs.begin() );
        }

        void Render()
        {
            auto now = std::chrono::steady_clock::now();
            auto* dl = ImGui::GetForegroundDrawList();
            ImVec2 display = ImGui::GetIO().DisplaySize;

            m_notifs.erase(
                std::remove_if( m_notifs.begin(), m_notifs.end(), [&]( const Notification& n )
                {
                    return std::chrono::duration<float>( now - n.created ).count() > n.lifetime;
                } ),
                m_notifs.end() );

            float yOff = 0.f;
            for ( int i = static_cast<int>( m_notifs.size() ) - 1; i >= 0; --i )
            {
                auto& n = m_notifs[i];
                float age = std::chrono::duration<float>( now - n.created ).count();

                float alpha = 1.f;
                if ( age < 0.3f )
                    alpha = age / 0.3f;
                else if ( age > n.lifetime - 0.5f )
                    alpha = ( n.lifetime - age ) / 0.5f;
                if ( alpha < 0.f ) alpha = 0.f;

                ImVec2 textSize = ImGui::CalcTextSize( n.message.c_str() );
                float w = textSize.x + 30.f;
                float h = textSize.y + 16.f;
                if ( w < 200.f ) w = 200.f;

                float x = display.x - w - 15.f;
                float y = display.y - 15.f - yOff - h;

                ImU32 barCol = GetBarColor( n.type, alpha );
                ImU32 bgCol  = IM_COL32( 20, 20, 20, static_cast<int>( 200 * alpha ) );
                ImU32 txtCol = IM_COL32( 230, 230, 230, static_cast<int>( 255 * alpha ) );

                dl->AddRectFilled( ImVec2( x, y ), ImVec2( x + w, y + h ), bgCol, 6.f );
                dl->AddRectFilled( ImVec2( x, y ), ImVec2( x + 4.f, y + h ), barCol, 6.f, ImDrawFlags_RoundCornersLeft );
                dl->AddText( ImVec2( x + 14.f, y + 8.f ), txtCol, n.message.c_str() );

                yOff += h + 6.f;
            }
        }

        const std::vector<Notification>& Recent() const { return m_notifs; }

    private:
        Notifications() = default;
        std::vector<Notification> m_notifs;

        static ImU32 GetBarColor( NotifType type, float alpha )
        {
            int a = static_cast<int>( 255 * alpha );
            switch ( type )
            {
            case NotifType::Info:    return IM_COL32( 0, 200, 255, a );
            case NotifType::Success: return IM_COL32( 0, 220, 80, a );
            case NotifType::Warning: return IM_COL32( 255, 200, 0, a );
            case NotifType::Error:   return IM_COL32( 255, 50, 50, a );
            }
            return IM_COL32( 200, 200, 200, a );
        }
    };

    inline Notifications& g_Notifs = Notifications::Get();
}
