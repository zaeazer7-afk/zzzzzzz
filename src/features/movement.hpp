#pragma once
#include <core/scheduler.hpp>
#include <Windows.h>
#include <string>
#include <atomic>

namespace Features
{
    class Movement
    {
    public:
        static Movement& Get() { static Movement inst; return inst; }

        bool  SuperRun       = false;
        float RunSpeed       = 3.f;
        bool  SuperJump      = false;
        bool  InfiniteStamina = false;
        bool  NoRagdoll      = false;
        bool  Noclip         = false;
        float NoclipSpeed    = 2.f;
        bool  FlyHack        = false;
        float FlySpeed       = 3.f;
        bool  SwimFast       = false;
        bool  Seatbelt       = false;
        bool  NoClipSmooth   = false;

        void Update()
        {
            if (SuperRun)
            {
                std::string code = "SetRunSprintMultiplierForPlayer(PlayerId(), " + std::to_string(RunSpeed) + ")";
                Core::g_Scheduler.Execute(code.c_str(), "super_run");
            }

            if (SuperJump)
            {
                Core::g_Scheduler.Execute("SetSuperJumpThisFrame(PlayerId())", "super_jump");
            }

            if (InfiniteStamina)
            {
                Core::g_Scheduler.Execute("RestorePlayerStamina(PlayerId(), 100.0)", "inf_stamina");
            }

            if (NoRagdoll)
            {
                Core::g_Scheduler.Execute(R"(
                    local ped = PlayerPedId()
                    SetPedCanRagdoll(ped, false)
                    SetPedCanRagdollFromPlayerImpact(ped, false)
                )", "no_ragdoll");
            }

            if (Noclip)
            {
                UpdateNoclip();
            }

            if (FlyHack)
            {
                UpdateFlyHack();
            }

            if (SwimFast)
            {
                Core::g_Scheduler.Execute("SetSwimMultiplierForPlayer(PlayerId(), 5.0)", "swim_fast");
            }

            if (Seatbelt)
            {
                Core::g_Scheduler.Execute(R"(
                    local ped = PlayerPedId()
                    if IsPedInAnyVehicle(ped, false) then
                        SetPedCanBeKnockedOffVehicle(ped, 1)
                        SetPedConfigFlag(ped, 32, false)
                    end
                )", "seatbelt");
            }
        }

    private:
        Movement() = default;

        void UpdateNoclip()
        {
            float speed = NoclipSpeed;
            std::string moveCode;

            // Direction from camera heading + pitch
            if (GetAsyncKeyState('W') & 0x8000) moveCode = "fwd";
            else if (GetAsyncKeyState('S') & 0x8000) moveCode = "back";
            else if (GetAsyncKeyState('A') & 0x8000) moveCode = "left";
            else if (GetAsyncKeyState('D') & 0x8000) moveCode = "right";

            if (GetAsyncKeyState(VK_SHIFT) & 0x8000) speed *= 3.f;
            if (GetAsyncKeyState(VK_CONTROL) & 0x8000) speed *= 0.3f;

            std::string code = R"(
                local ped = PlayerPedId()
                SetEntityCollision(ped, false, false)
                FreezeEntityPosition(ped, true)
                local cam = GetGameplayCamRot(2)
                local heading = cam.z
                local pitch = cam.x
                local c = GetEntityCoords(ped)
                local speed = )" + std::to_string(speed) + R"(
                local rad = heading * 3.14159 / 180.0
                local prad = pitch * 3.14159 / 180.0
                local dx, dy, dz = 0.0, 0.0, 0.0
            )";

            if (moveCode == "fwd")
                code += "dx = -math.sin(rad)*speed*math.cos(prad) dy = math.cos(rad)*speed*math.cos(prad) dz = math.sin(prad)*speed\n";
            else if (moveCode == "back")
                code += "dx = math.sin(rad)*speed*math.cos(prad) dy = -math.cos(rad)*speed*math.cos(prad) dz = -math.sin(prad)*speed\n";
            else if (moveCode == "left")
                code += "dx = math.cos(rad)*speed dy = math.sin(rad)*speed\n";
            else if (moveCode == "right")
                code += "dx = -math.cos(rad)*speed dy = -math.sin(rad)*speed\n";

            if (GetAsyncKeyState(VK_SPACE) & 0x8000) code += "dz = dz + speed\n";
            if (GetAsyncKeyState('X') & 0x8000) code += "dz = dz - speed\n";

            code += "SetEntityCoordsNoOffset(ped, c.x+dx, c.y+dy, c.z+dz, false, false, false)\n";

            Core::g_Scheduler.Execute(code.c_str(), "noclip");
        }

        void UpdateFlyHack()
        {
            float speed = FlySpeed;
            if (GetAsyncKeyState(VK_SHIFT) & 0x8000) speed *= 3.f;

            std::string code = R"(
                local ped = PlayerPedId()
                SetEntityCollision(ped, true, true)
                FreezeEntityPosition(ped, false)
                ClearPedTasksImmediately(ped)
                local cam = GetGameplayCamRot(2)
                local rad = cam.z * 3.14159 / 180.0
                local prad = cam.x * 3.14159 / 180.0
                local speed = )" + std::to_string(speed) + R"(
                local vx, vy, vz = 0.0, 0.0, 0.0
            )";

            if (GetAsyncKeyState('W') & 0x8000)
                code += "vx = -math.sin(rad)*speed*math.cos(prad) vy = math.cos(rad)*speed*math.cos(prad) vz = math.sin(prad)*speed\n";
            else if (GetAsyncKeyState('S') & 0x8000)
                code += "vx = math.sin(rad)*speed vy = -math.cos(rad)*speed\n";

            if (GetAsyncKeyState(VK_SPACE) & 0x8000) code += "vz = vz + speed\n";

            code += "SetEntityVelocity(ped, vx, vy, vz)\n";

            Core::g_Scheduler.Execute(code.c_str(), "fly_hack");
        }
    };

    inline Movement& g_Movement = Movement::Get();
}
