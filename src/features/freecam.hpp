#pragma once
#include <core/scheduler.hpp>
#include <Windows.h>
#include <string>
#include <atomic>

namespace Features
{
    class Freecam
    {
    public:
        static Freecam& Get() { static Freecam inst; return inst; }

        bool  Enabled    = false;
        float Speed      = 1.f;
        float FastMult   = 3.f;
        float SlowMult   = 0.3f;
        bool  TPOnExit   = true;
        int   ToggleKey  = VK_F3;

        void Update()
        {
            bool keyDown = (GetAsyncKeyState(ToggleKey) & 0x8000) != 0;
            if (keyDown && !m_lastKey)
            {
                Enabled = !Enabled;
                if (Enabled) Enter();
                else Exit();
            }
            m_lastKey = keyDown;

            if (!Enabled || !m_active) return;

            float spd = Speed;
            if (GetAsyncKeyState(VK_SHIFT) & 0x8000) spd *= FastMult;
            if (GetAsyncKeyState(VK_CONTROL) & 0x8000) spd *= SlowMult;

            std::string move;
            if (GetAsyncKeyState('W') & 0x8000) move = "fwd";
            else if (GetAsyncKeyState('S') & 0x8000) move = "back";

            std::string strafe;
            if (GetAsyncKeyState('A') & 0x8000) strafe = "left";
            else if (GetAsyncKeyState('D') & 0x8000) strafe = "right";

            std::string vert;
            if (GetAsyncKeyState(VK_SPACE) & 0x8000) vert = "up";
            else if (GetAsyncKeyState('X') & 0x8000) vert = "down";

            std::string code = R"(
                local cam = )" + std::to_string(m_camHandle) + R"(
                if not DoesCamExist(cam) then return end
                local rot = GetCamRot(cam, 2)
                local pos = GetCamCoord(cam)
                local speed = )" + std::to_string(spd) + R"(
                local rad = rot.z * 3.14159 / 180.0
                local prad = rot.x * 3.14159 / 180.0
                local dx, dy, dz = 0.0, 0.0, 0.0
            )";

            if (move == "fwd")
                code += "dx = -math.sin(rad)*speed*math.cos(prad) dy = math.cos(rad)*speed*math.cos(prad) dz = math.sin(prad)*speed\n";
            else if (move == "back")
                code += "dx = math.sin(rad)*speed*math.cos(prad) dy = -math.cos(rad)*speed*math.cos(prad) dz = -math.sin(prad)*speed\n";

            if (strafe == "left")
                code += "dx = dx + math.cos(rad)*speed dy = dy + math.sin(rad)*speed\n";
            else if (strafe == "right")
                code += "dx = dx - math.cos(rad)*speed dy = dy - math.sin(rad)*speed\n";

            if (vert == "up") code += "dz = dz + speed\n";
            else if (vert == "down") code += "dz = dz - speed\n";

            code += R"(
                SetCamCoord(cam, pos.x + dx, pos.y + dy, pos.z + dz)
                -- Point cam with mouse (game handles rotation via SetCamRot controlled by mouse input from game)
            )";

            Core::g_Scheduler.Execute(code.c_str(), "freecam_move");
        }

    private:
        Freecam() = default;

        void Enter()
        {
            auto res = Core::g_Scheduler.Execute(R"(
                local cam = CreateCam('DEFAULT_SCRIPTED_CAMERA', true)
                local ped = PlayerPedId()
                local pos = GetEntityCoords(ped)
                local rot = GetEntityRotation(ped)
                SetCamCoord(cam, pos.x, pos.y, pos.z + 1.5)
                SetCamRot(cam, rot.x, rot.y, rot.z, 2)
                SetCamActive(cam, true)
                RenderScriptCams(true, true, 500, true, true)
                FreezeEntityPosition(ped, true)
                SetEntityVisible(ped, false, false)
                SetEntityCollision(ped, false, false)
                return cam
            )", "freecam_enter");

            if (res.Ok && !res.Output.empty())
            {
                try { m_camHandle = std::stoi(res.Output); } catch (...) { m_camHandle = 0; }
            }
            m_active = (m_camHandle != 0);
        }

        void Exit()
        {
            if (m_camHandle)
            {
                std::string code;
                if (TPOnExit)
                {
                    code = R"(
                        local cam = )" + std::to_string(m_camHandle) + R"(
                        local pos = GetCamCoord(cam)
                        local ped = PlayerPedId()
                        SetEntityCoordsNoOffset(ped, pos.x, pos.y, pos.z, false, false, false)
                        FreezeEntityPosition(ped, false)
                        SetEntityVisible(ped, true, false)
                        SetEntityCollision(ped, true, true)
                        RenderScriptCams(false, true, 500, true, true)
                        DestroyCam(cam, false)
                    )";
                }
                else
                {
                    code = R"(
                        local cam = )" + std::to_string(m_camHandle) + R"(
                        local ped = PlayerPedId()
                        FreezeEntityPosition(ped, false)
                        SetEntityVisible(ped, true, false)
                        SetEntityCollision(ped, true, true)
                        RenderScriptCams(false, true, 500, true, true)
                        DestroyCam(cam, false)
                    )";
                }
                Core::g_Scheduler.Execute(code.c_str(), "freecam_exit");
            }
            m_camHandle = 0;
            m_active = false;
        }

        bool m_lastKey   = false;
        bool m_active    = false;
        int  m_camHandle = 0;
    };

    inline Freecam& g_Freecam = Freecam::Get();
}
