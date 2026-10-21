#pragma once
#include <imgui.h>
#include <core/scheduler.hpp>
#include <core/config.hpp>
#include <features/esp.hpp>
#include <utils/math.hpp>
#include <Windows.h>
#include <atomic>
#include <cmath>

namespace Features
{
    class Aimbot
    {
    public:
        static Aimbot& Get() { static Aimbot inst; return inst; }

        // Settings
        bool  Enabled      = false;
        int   Hotkey       = VK_RBUTTON;   // Right mouse default
        bool  HoldToAim    = true;
        float FOV          = 150.f;        // Screen pixels
        bool  DrawFOV      = true;
        float Smoothing    = 5.f;          // 1 = instant, higher = smoother
        int   TargetBone   = 0;            // 0=head, 1=neck, 2=chest, 3=pelvis
        int   Priority     = 0;            // 0=closest to crosshair, 1=closest distance, 2=lowest HP
        bool  VisCheck     = true;
        bool  StickyTarget = true;
        float ReacquireDelay = 0.3f;
        ImVec4 FOVColor    = ImVec4(1.f, 1.f, 1.f, 0.3f);

        void Update()
        {
            if (!Enabled) return;

            bool keyDown = (GetAsyncKeyState(Hotkey) & 0x8000) != 0;
            if (HoldToAim && !keyDown)
            {
                m_currentTarget = -1;
                return;
            }
            if (!HoldToAim)
            {
                if (keyDown && !m_lastKeyState) m_toggled = !m_toggled;
                m_lastKeyState = keyDown;
                if (!m_toggled) { m_currentTarget = -1; return; }
            }

            auto& esp = g_ESP;
            auto& players = esp.GetPlayers();
            if (players.empty()) return;

            ImVec2 screen = ImGui::GetIO().DisplaySize;
            Utils::Vec2 center(screen.x / 2.f, screen.y / 2.f);

            int bestIdx = -1;
            float bestScore = 999999.f;

            // Sticky target — keep current if still valid
            if (StickyTarget && m_currentTarget >= 0)
            {
                for (int i = 0; i < (int)players.size(); i++)
                {
                    if (players[i].ServerId == m_currentTarget && players[i].OnScreen && players[i].IsAlive)
                    {
                        bestIdx = i;
                        break;
                    }
                }
            }

            if (bestIdx < 0)
            {
                for (int i = 0; i < (int)players.size(); i++)
                {
                    auto& p = players[i];
                    if (!p.OnScreen || !p.IsAlive) continue;
                    if (p.Distance < 1.f || p.Distance > g_ESP.MaxDistance) continue;

                    // Get target bone screen pos
                    int boneIdx = GetBoneIndex();
                    float tx = p.BoneValid[boneIdx] ? p.BoneScreen[boneIdx][0] : p.ScreenX;
                    float ty = p.BoneValid[boneIdx] ? p.BoneScreen[boneIdx][1] : p.ScreenY;

                    float fovDist = center.Distance(Utils::Vec2(tx, ty));
                    if (fovDist > FOV) continue;

                    float score;
                    switch (Priority)
                    {
                    case 1: score = p.Distance; break;
                    case 2: score = p.Health; break;
                    default: score = fovDist; break;
                    }

                    if (score < bestScore)
                    {
                        bestScore = score;
                        bestIdx = i;
                    }
                }
            }

            if (bestIdx < 0) { m_currentTarget = -1; return; }

            auto& target = players[bestIdx];
            m_currentTarget = target.ServerId;

            int boneIdx = GetBoneIndex();
            float tx = target.BoneValid[boneIdx] ? target.BoneScreen[boneIdx][0] : target.ScreenX;
            float ty = target.BoneValid[boneIdx] ? target.BoneScreen[boneIdx][1] : target.ScreenY;

            // Calculate mouse delta with smoothing
            float dx = tx - center.x;
            float dy = ty - center.y;

            float smooth = (Smoothing < 1.f) ? 1.f : Smoothing;
            dx /= smooth;
            dy /= smooth;

            // Apply mouse movement
            if (fabsf(dx) > 0.5f || fabsf(dy) > 0.5f)
            {
                INPUT input{};
                input.type = INPUT_MOUSE;
                input.mi.dwFlags = MOUSEEVENTF_MOVE;
                input.mi.dx = (LONG)dx;
                input.mi.dy = (LONG)dy;
                SendInput(1, &input, sizeof(INPUT));
            }
        }

        void RenderFOVCircle()
        {
            if (!Enabled || !DrawFOV) return;

            auto* drawList = ImGui::GetBackgroundDrawList();
            ImVec2 screen = ImGui::GetIO().DisplaySize;
            ImVec2 center(screen.x / 2.f, screen.y / 2.f);

            drawList->AddCircle(center, FOV,
                ImGui::ColorConvertFloat4ToU32(FOVColor), 64, 1.5f);
        }

    private:
        Aimbot() = default;

        int GetBoneIndex() const
        {
            return TargetBone;
        }

        int  m_currentTarget = -1;
        bool m_lastKeyState  = false;
        bool m_toggled       = false;
    };

    inline Aimbot& g_Aimbot = Aimbot::Get();
}
