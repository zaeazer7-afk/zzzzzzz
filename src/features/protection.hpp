#pragma once
#include <core/scheduler.hpp>
#include <string>
#include <chrono>

namespace Features
{
    class Protection
    {
    public:
        static Protection& Get() { static Protection inst; return inst; }

        bool AntiExplosion    = false;
        bool AntiFreeze       = false;
        bool AntiCage         = false;
        bool AntiFall         = false;
        bool AntiRagdoll      = false;
        bool AntiPtfx         = false;
        bool AntiAttach       = false;
        bool AntiTeleport     = false;
        bool AntiWeaponRemove = false;
        bool AntiInvisible    = false;

        void Update()
        {
            if (AntiExplosion)
            {
                Core::g_Scheduler.Execute(R"(
                    local ped = PlayerPedId()
                    local c = GetEntityCoords(ped)
                    if IsExplosionInArea(-1, c.x - 10.0, c.y - 10.0, c.z - 10.0, c.x + 10.0, c.y + 10.0, c.z + 10.0) then
                        local h = GetEntityHeading(ped)
                        local rad = math.rad(h)
                        SetEntityCoordsNoOffset(ped, c.x - math.sin(rad)*15.0, c.y + math.cos(rad)*15.0, c.z, false, false, false)
                    end
                )", "anti_explode");
            }

            if (AntiFreeze)
            {
                Core::g_Scheduler.Execute(R"(
                    local ped = PlayerPedId()
                    if IsEntityPositionFrozen(ped) then
                        FreezeEntityPosition(ped, false)
                    end
                    SetEntityCollision(ped, true, true)
                )", "anti_freeze");
            }

            if (AntiCage)
            {
                Core::g_Scheduler.Execute(R"(
                    local ped = PlayerPedId()
                    local c = GetEntityCoords(ped)
                    local handle, obj = FindFirstObject()
                    local found = true
                    while found do
                        local oc = GetEntityCoords(obj)
                        local d = #(c - oc)
                        if d < 5.0 then
                            local model = GetEntityModel(obj)
                            if model == GetHashKey('prop_gold_cont_01') or
                               model == GetHashKey('prop_container_01a') or
                               model == GetHashKey('prop_container_01b') or
                               model == GetHashKey('prop_container_01mb') or
                               model == GetHashKey('prop_contnr_pile_01a') or
                               model == GetHashKey('stt_prop_stunt_tube_s') then
                                SetEntityAsMissionEntity(obj, true, true)
                                DeleteEntity(obj)
                            end
                        end
                        found, obj = FindNextObject(handle)
                    end
                    EndFindObject(handle)
                )", "anti_cage");
            }

            if (AntiFall)
            {
                Core::g_Scheduler.Execute(R"(
                    local ped = PlayerPedId()
                    local c = GetEntityCoords(ped)
                    if GetEntityHeightAboveGround(ped) > 100.0 and IsPedFalling(ped) then
                        SetEntityVelocity(ped, 0.0, 0.0, -1.0)
                        SetPedParachuteTaskForceStart(ped)
                    end
                    if c.z < -50.0 then
                        SetEntityCoordsNoOffset(ped, c.x, c.y, 200.0, false, false, false)
                    end
                )", "anti_fall");
            }

            if (AntiRagdoll)
            {
                Core::g_Scheduler.Execute(R"(
                    local ped = PlayerPedId()
                    SetPedCanRagdoll(ped, false)
                    SetPedCanRagdollFromPlayerImpact(ped, false)
                    SetPedRagdollOnCollision(ped, false)
                )", "anti_ragdoll");
            }

            if (AntiPtfx)
            {
                Core::g_Scheduler.Execute(R"(
                    local c = GetEntityCoords(PlayerPedId())
                    RemoveParticleFxInRange(c.x, c.y, c.z, 15.0)
                )", "anti_ptfx");
            }

            if (AntiAttach)
            {
                Core::g_Scheduler.Execute(R"(
                    local ped = PlayerPedId()
                    local c = GetEntityCoords(ped)
                    local handle, obj = FindFirstObject()
                    local found = true
                    while found do
                        if IsEntityAttachedToEntity(obj, ped) then
                            DetachEntity(obj, true, true)
                            SetEntityAsMissionEntity(obj, true, true)
                            DeleteEntity(obj)
                        end
                        found, obj = FindNextObject(handle)
                    end
                    EndFindObject(handle)
                )", "anti_attach");
            }

            if (AntiTeleport)
            {
                Core::g_Scheduler.Execute(R"lua(
                    local ped = PlayerPedId()
                    local c = GetEntityCoords(ped)
                    local stored = GetResourceKvpString("__tri_lastpos")
                    if stored and stored ~= "" then
                        local px,py,pz = stored:match("([^|]+)|([^|]+)|([^|]+)")
                        px,py,pz = tonumber(px),tonumber(py),tonumber(pz)
                        if px then
                            local dx,dy,dz = c.x-px, c.y-py, c.z-pz
                            local dist = math.sqrt(dx*dx+dy*dy+dz*dz)
                            if dist > 50.0 and not IsPedInAnyVehicle(ped, false) then
                                SetEntityCoordsNoOffset(ped, px, py, pz, false, false, false)
                            end
                        end
                    end
                    SetResourceKvp("__tri_lastpos", string.format("%.2f|%.2f|%.2f", c.x, c.y, c.z))
                )lua", "anti_tp");
            }

            if (AntiWeaponRemove)
            {
                Core::g_Scheduler.Execute(R"(
                    local ped = PlayerPedId()
                    local weapons = {
                        `WEAPON_PISTOL`,`WEAPON_COMBATPISTOL`,`WEAPON_APPISTOL`,`WEAPON_PISTOL50`,
                        `WEAPON_MICROSMG`,`WEAPON_SMG`,`WEAPON_ASSAULTSMG`,`WEAPON_MG`,`WEAPON_COMBATMG`,
                        `WEAPON_ASSAULTRIFLE`,`WEAPON_CARBINERIFLE`,`WEAPON_ADVANCEDRIFLE`,`WEAPON_SPECIALCARBINE`,
                        `WEAPON_PUMPSHOTGUN`,`WEAPON_SAWNOFFSHOTGUN`,`WEAPON_ASSAULTSHOTGUN`,
                        `WEAPON_SNIPERRIFLE`,`WEAPON_HEAVYSNIPER`,`WEAPON_MARKSMANRIFLE`,
                        `WEAPON_RPG`,`WEAPON_GRENADELAUNCHER`,`WEAPON_MINIGUN`
                    }
                    local current = {}
                    for _,wh in ipairs(weapons) do
                        if HasPedGotWeapon(ped, wh, false) then current[#current+1] = wh end
                    end
                    local stored = GetResourceKvpString("__tri_weaps")
                    if #current > 0 then
                        local s = ""
                        for i,w in ipairs(current) do
                            s = s .. (i>1 and "," or "") .. tostring(w)
                        end
                        SetResourceKvp("__tri_weaps", s)
                    elseif stored and stored ~= "" then
                        for w in stored:gmatch("[^,]+") do
                            local wh = tonumber(w)
                            if wh then GiveWeaponToPed(ped, wh, 9999, false, true) end
                        end
                    end
                )", "anti_weapon_rm");
            }

            if (AntiInvisible)
            {
                Core::g_Scheduler.Execute(R"(
                    local ped = PlayerPedId()
                    if not IsEntityVisible(ped) then
                        SetEntityVisible(ped, true, false)
                    end
                )", "anti_invis");
            }
        }

        void DeleteNearbyObjects(float radius = 15.f)
        {
            std::string code = R"(
                local ped = PlayerPedId()
                local c = GetEntityCoords(ped)
                local handle, obj = FindFirstObject()
                local found = true
                while found do
                    local oc = GetEntityCoords(obj)
                    if #(c - oc) < )" + std::to_string(radius) + R"( then
                        SetEntityAsMissionEntity(obj, true, true)
                        DeleteEntity(obj)
                    end
                    found, obj = FindNextObject(handle)
                end
                EndFindObject(handle)
            )";
            Core::g_Scheduler.ExecuteSafe(code.c_str(), "del_objects");
        }

        void DeleteNearbyVehicles(float radius = 15.f)
        {
            std::string code = R"(
                local ped = PlayerPedId()
                local c = GetEntityCoords(ped)
                local myVeh = GetVehiclePedIsIn(ped, false)
                local handle, veh = FindFirstVehicle()
                local found = true
                while found do
                    local vc = GetEntityCoords(veh)
                    if #(c - vc) < )" + std::to_string(radius) + R"( and veh ~= myVeh then
                        SetEntityAsMissionEntity(veh, true, true)
                        DeleteEntity(veh)
                    end
                    found, veh = FindNextVehicle(handle)
                end
                EndFindVehicle(handle)
            )";
            Core::g_Scheduler.ExecuteSafe(code.c_str(), "del_vehicles");
        }

        void CleanupArea()
        {
            Core::g_Scheduler.ExecuteSafe(R"(
                local c = GetEntityCoords(PlayerPedId())
                ClearAreaOfEverything(c.x, c.y, c.z, 200.0, false, false, false, false)
            )", "cleanup_area");
        }

    private:
        Protection() = default;
    };

    inline Protection& g_Protection = Protection::Get();
}
