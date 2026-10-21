#pragma once

#include <core/scheduler.hpp>
#include <string>

namespace Features
{
    class WeaponMods
    {
    public:
        static WeaponMods& Get()
        {
            static WeaponMods inst;
            return inst;
        }

        bool GravityGun  = false;
        bool DeleteGun   = false;
        bool RepairGun   = false;
        bool TeleportGun = false;
        bool MoneyGun    = false;

        float GravityForce = 20.f;

        void Update()
        {
            if ( !GravityGun && !DeleteGun && !RepairGun && !TeleportGun && !MoneyGun )
                return;

            if ( GravityGun )
            {
                Core::g_Scheduler.Execute( R"(
                    local ped = PlayerPedId()
                    if IsPlayerFreeAiming(PlayerId()) then
                        local hit, ent = GetEntityPlayerIsFreeAimingAt(PlayerId())
                        if hit and ent ~= 0 and ent ~= ped then
                            if IsDisabledControlPressed(0, 24) then
                                local pCoords = GetEntityCoords(ped)
                                local eCoords = GetEntityCoords(ent)
                                local dx = pCoords.x - eCoords.x
                                local dy = pCoords.y - eCoords.y
                                local dz = pCoords.z - eCoords.z
                                local dist = math.sqrt(dx*dx + dy*dy + dz*dz)
                                if dist > 0 then
                                    local f = )" + std::to_string( GravityForce ) + R"( / dist
                                    ApplyForceToEntity(ent, 1, dx*f, dy*f, dz*f, 0.0, 0.0, 0.0, 0, false, true, true, false, true)
                                end
                            elseif IsDisabledControlPressed(0, 25) then
                                local pCoords = GetEntityCoords(ped)
                                local eCoords = GetEntityCoords(ent)
                                local dx = eCoords.x - pCoords.x
                                local dy = eCoords.y - pCoords.y
                                local dz = eCoords.z - pCoords.z
                                local dist = math.sqrt(dx*dx + dy*dy + dz*dz)
                                if dist > 0 then
                                    local f = )" + std::to_string( GravityForce ) + R"( / dist
                                    ApplyForceToEntity(ent, 1, dx*f, dy*f, dz*f, 0.0, 0.0, 0.0, 0, false, true, true, false, true)
                                end
                            end
                        end
                    end
                )" );
            }

            if ( DeleteGun )
            {
                Core::g_Scheduler.Execute( R"(
                    if IsPlayerFreeAiming(PlayerId()) and IsDisabledControlJustPressed(0, 24) then
                        local hit, ent = GetEntityPlayerIsFreeAimingAt(PlayerId())
                        if hit and ent ~= 0 and ent ~= PlayerPedId() then
                            SetEntityAsMissionEntity(ent, true, true)
                            DeleteEntity(ent)
                        end
                    end
                )" );
            }

            if ( RepairGun )
            {
                Core::g_Scheduler.Execute( R"(
                    if IsPlayerFreeAiming(PlayerId()) and IsDisabledControlJustPressed(0, 24) then
                        local hit, ent = GetEntityPlayerIsFreeAimingAt(PlayerId())
                        if hit and ent ~= 0 and IsEntityAVehicle(ent) then
                            SetVehicleFixed(ent)
                            SetVehicleDeformationFixed(ent)
                            SetVehicleUndriveable(ent, false)
                            SetVehicleEngineHealth(ent, 1000.0)
                            SetVehicleBodyHealth(ent, 1000.0)
                            SetVehiclePetrolTankHealth(ent, 1000.0)
                        end
                    end
                )" );
            }

            if ( TeleportGun )
            {
                Core::g_Scheduler.Execute( R"(
                    if IsPlayerFreeAiming(PlayerId()) and IsDisabledControlJustPressed(0, 24) then
                        local ped = PlayerPedId()
                        local camRot = GetGameplayCamRot(2)
                        local camCoord = GetGameplayCamCoord()
                        local dir = {
                            x = -math.sin(math.rad(camRot.z)) * math.abs(math.cos(math.rad(camRot.x))),
                            y = math.cos(math.rad(camRot.z)) * math.abs(math.cos(math.rad(camRot.x))),
                            z = math.sin(math.rad(camRot.x))
                        }
                        local dest = {
                            x = camCoord.x + dir.x * 1000.0,
                            y = camCoord.y + dir.y * 1000.0,
                            z = camCoord.z + dir.z * 1000.0
                        }
                        local ray = StartShapeTestRay(camCoord.x, camCoord.y, camCoord.z, dest.x, dest.y, dest.z, -1, ped, 0)
                        local _, hit, endCoords, _, _ = GetShapeTestResult(ray)
                        if hit then
                            SetEntityCoords(ped, endCoords.x, endCoords.y, endCoords.z + 1.0, false, false, false, false)
                        end
                    end
                )" );
            }

            if ( MoneyGun )
            {
                Core::g_Scheduler.Execute( R"(
                    if IsPlayerFreeAiming(PlayerId()) and IsDisabledControlJustPressed(0, 24) then
                        local hit, ent = GetEntityPlayerIsFreeAimingAt(PlayerId())
                        if hit and ent ~= 0 then
                            local c = GetEntityCoords(ent)
                            for i = 1, 5 do
                                local ox = math.random(-2, 2) * 1.0
                                local oy = math.random(-2, 2) * 1.0
                                CreateAmbientPickup(0x1B1240D0, c.x + ox, c.y + oy, c.z + 1.0, 0, 2000, 1, false, true)
                            end
                        end
                    end
                )" );
            }
        }

    private:
        WeaponMods() = default;
    };

    inline WeaponMods& g_WeaponMods = WeaponMods::Get();
}
