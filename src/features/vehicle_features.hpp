#pragma once
#include <core/scheduler.hpp>
#include <Windows.h>
#include <string>

namespace Features
{
    class VehicleFeatures
    {
    public:
        static VehicleFeatures& Get() { static VehicleFeatures inst; return inst; }

        bool  VehicleGodMode    = false;
        bool  HornBoost         = false;
        float HornBoostForce    = 50.f;
        bool  DriftMode         = false;
        bool  RainbowCar        = false;
        bool  SpeedBoost        = false;
        int   SpeedBoostKey     = VK_NUMPAD9;
        float SpeedBoostAmount  = 30.f;
        bool  StickyWheels      = false;
        bool  InvisibleVehicle  = false;
        bool  VehicleFly        = false;
        float VehicleFlySpeed   = 30.f;
        bool  Seatbelt          = false;
        bool  AutoPilot         = false;
        bool  InfiniteNitro     = false;

        void Update()
        {
            if (VehicleGodMode)
            {
                Core::g_Scheduler.Execute(R"(
                    local veh = GetVehiclePedIsIn(PlayerPedId(), false)
                    if veh ~= 0 then
                        SetEntityInvincible(veh, true)
                        SetEntityProofs(veh, true, true, true, true, true, true, true, true)
                        SetVehicleTyresCanBurst(veh, false)
                        SetVehicleCanBeVisiblyDamaged(veh, false)
                        SetVehicleEngineOn(veh, true, true, false)
                    end
                )", "veh_god");
            }

            if (HornBoost)
            {
                std::string code = R"(
                    local ped = PlayerPedId()
                    local veh = GetVehiclePedIsIn(ped, false)
                    if veh ~= 0 and GetPedInVehicleSeat(veh, -1) == ped then
                        if IsHornActive(veh) then
                            local speed = GetEntitySpeed(veh)
                            SetVehicleForwardSpeed(veh, speed + )" + std::to_string(HornBoostForce) + R"()
                        end
                    end
                )";
                Core::g_Scheduler.Execute(code.c_str(), "horn_boost");
            }

            if (DriftMode)
            {
                Core::g_Scheduler.Execute(R"(
                    local veh = GetVehiclePedIsIn(PlayerPedId(), false)
                    if veh ~= 0 then
                        SetVehicleReduceGrip(veh, true)
                        SetVehicleHandlingFloat(veh, 'CHandlingData', 'fTractionCurveMin', 0.1)
                    end
                )", "drift_mode");
            }

            if (RainbowCar)
            {
                Core::g_Scheduler.Execute(R"(
                    local veh = GetVehiclePedIsIn(PlayerPedId(), false)
                    if veh ~= 0 then
                        local r = math.random(0, 255)
                        local g = math.random(0, 255)
                        local b = math.random(0, 255)
                        SetVehicleCustomPrimaryColour(veh, r, g, b)
                        SetVehicleCustomSecondaryColour(veh, r, g, b)
                    end
                )", "rainbow_car");
            }

            if (SpeedBoost)
            {
                bool keyDown = (GetAsyncKeyState(SpeedBoostKey) & 0x8000) != 0;
                if (keyDown)
                {
                    std::string code = R"(
                        local veh = GetVehiclePedIsIn(PlayerPedId(), false)
                        if veh ~= 0 then
                            local speed = GetEntitySpeed(veh)
                            SetVehicleForwardSpeed(veh, speed + )" + std::to_string(SpeedBoostAmount) + R"()
                        end
                    )";
                    Core::g_Scheduler.Execute(code.c_str(), "speed_boost");
                }
            }

            if (InvisibleVehicle)
            {
                Core::g_Scheduler.Execute(R"(
                    local veh = GetVehiclePedIsIn(PlayerPedId(), false)
                    if veh ~= 0 then
                        SetEntityVisible(veh, false, false)
                    end
                )", "invis_veh");
            }

            if (VehicleFly)
            {
                UpdateVehicleFly();
            }

            if (InfiniteNitro)
            {
                Core::g_Scheduler.Execute(R"(
                    local veh = GetVehiclePedIsIn(PlayerPedId(), false)
                    if veh ~= 0 then
                        SetVehicleNitroEnabled(veh, true)
                        SetVehicleBoostActive(veh, true)
                    end
                )", "inf_nitro");
            }

            if (AutoPilot)
            {
                Core::g_Scheduler.Execute(R"(
                    local ped = PlayerPedId()
                    local veh = GetVehiclePedIsIn(ped, false)
                    if veh ~= 0 then
                        local blip = GetFirstBlipInfoId(8)
                        if DoesBlipExist(blip) then
                            local c = GetBlipInfoIdCoord(blip)
                            TaskVehicleDriveToCoordLongrange(ped, veh, c.x, c.y, c.z, 30.0, 786603, 5.0)
                        end
                    end
                )", "auto_pilot");
            }

            if (StickyWheels)
            {
                Core::g_Scheduler.Execute(R"(
                    local veh = GetVehiclePedIsIn(PlayerPedId(), false)
                    if veh ~= 0 then
                        SetVehicleOnGroundProperly(veh)
                    end
                )", "sticky_wheels");
            }
        }

    private:
        VehicleFeatures() = default;

        void UpdateVehicleFly()
        {
            float speed = VehicleFlySpeed;
            if (GetAsyncKeyState(VK_SHIFT) & 0x8000) speed *= 2.f;

            std::string code = R"(
                local ped = PlayerPedId()
                local veh = GetVehiclePedIsIn(ped, false)
                if veh ~= 0 then
                    local rot = GetEntityRotation(veh)
                    local speed = )" + std::to_string(speed) + R"(
            )";

            if (GetAsyncKeyState(VK_NUMPAD8) & 0x8000)
                code += "ApplyForceToEntity(veh, 1, 0.0, 0.0, speed, 0.0, 0.0, 0.0, 0, true, true, true, false, true)\n";
            if (GetAsyncKeyState(VK_NUMPAD2) & 0x8000)
                code += "ApplyForceToEntity(veh, 1, 0.0, 0.0, -speed*0.5, 0.0, 0.0, 0.0, 0, true, true, true, false, true)\n";
            if (GetAsyncKeyState(VK_NUMPAD5) & 0x8000)
                code += "SetVehicleForwardSpeed(veh, speed)\n";

            code += "end\n";
            Core::g_Scheduler.Execute(code.c_str(), "veh_fly");
        }
    };

    inline VehicleFeatures& g_VehicleFeatures = VehicleFeatures::Get();
}
