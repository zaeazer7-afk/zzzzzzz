#pragma once
#include <core/scheduler.hpp>
#include <Windows.h>
#include <string>
#include <cmath>

namespace Features
{
    class WorldFeatures
    {
    public:
        static WorldFeatures& Get() { static WorldFeatures inst; return inst; }

        bool  GodMode          = false;
        bool  SemiGodMode      = false;
        bool  Invisible        = false;
        bool  AutoHeal         = false;
        bool  AutoRevive       = false;
        bool  NeverWanted      = false;
        bool  NoRagdoll        = false;
        bool  InfiniteStamina  = false;
        bool  SuperJump        = false;
        bool  SuperRun         = false;
        float RunSpeed         = 3.0f;
        bool  Noclip           = false;
        float NoclipSpeed      = 2.0f;
        int   NoclipToggleKey  = VK_F3;
        bool  NightVision      = false;
        bool  ThermalVision    = false;
        bool  Blackout         = false;
        bool  FastSwim         = false;
        bool  WantedLevelLock  = false;
        int   WantedLevel      = 0;
        bool  NoIdleCam        = false;
        bool  OverrideTime     = false;
        int   TimeHour         = 12;
        int   TimeMinute       = 0;
        bool  OverrideWeather  = false;
        int   WeatherIndex     = 0;

        const char* WeatherTypes[14] = {
            "CLEAR", "EXTRASUNNY", "CLOUDS", "OVERCAST", "RAIN",
            "CLEARING", "THUNDER", "SMOG", "FOGGY", "XMAS",
            "SNOWLIGHT", "BLIZZARD", "NEUTRAL", "HALLOWEEN"
        };
        static constexpr int WeatherCount = 14;

        void Update()
        {
            if (GodMode)
            {
                Core::g_Scheduler.Execute(R"(
                    local ped = PlayerPedId()
                    SetEntityInvincible(ped, true)
                    SetEntityProofs(ped, true, true, true, true, true, true, true, true)
                    SetPedCanBeKnockedOffVehicle(ped, 1)
                )", "god_mode");
            }

            if (SemiGodMode && !GodMode)
            {
                Core::g_Scheduler.Execute(R"(
                    local ped = PlayerPedId()
                    local hp = GetEntityHealth(ped)
                    if hp > 0 and hp < 150 then
                        SetEntityHealth(ped, 200)
                    end
                )", "semi_god");
            }

            if (Invisible)
            {
                Core::g_Scheduler.Execute(R"(
                    local ped = PlayerPedId()
                    SetEntityVisible(ped, false, false)
                    NetworkFadeOutEntity(ped, true, false)
                )", "invisible");
            }

            if (AutoHeal)
            {
                Core::g_Scheduler.Execute(R"(
                    local ped = PlayerPedId()
                    local hp = GetEntityHealth(ped)
                    local maxHp = GetEntityMaxHealth(ped)
                    if hp > 0 and hp < maxHp then
                        SetEntityHealth(ped, math.min(hp + 5, maxHp))
                    end
                )", "auto_heal");
            }

            if (AutoRevive)
            {
                Core::g_Scheduler.Execute(R"(
                    local ped = PlayerPedId()
                    if IsEntityDead(ped) then
                        local c = GetEntityCoords(ped)
                        NetworkResurrectLocalPlayer(c.x, c.y, c.z, GetEntityHeading(ped), true, false)
                        SetEntityHealth(ped, GetEntityMaxHealth(ped))
                        ClearPedBloodDamage(ped)
                    end
                )", "auto_revive");
            }

            if (NeverWanted)
            {
                Core::g_Scheduler.Execute(R"(
                    local pid = PlayerId()
                    SetPlayerWantedLevel(pid, 0, false)
                    SetMaxWantedLevel(0)
                    ClearPlayerWantedLevel(pid)
                )", "never_wanted");
            }

            if (NoRagdoll)
            {
                Core::g_Scheduler.Execute(R"(
                    local ped = PlayerPedId()
                    SetPedCanRagdoll(ped, false)
                    SetPedCanRagdollFromPlayerImpact(ped, false)
                    SetPedRagdollOnCollision(ped, false)
                )", "no_ragdoll");
            }

            if (InfiniteStamina)
            {
                Core::g_Scheduler.Execute(R"(
                    RestorePlayerStamina(PlayerId(), 100.0)
                    ResetPlayerStaminaDrainMultiplier(PlayerId())
                )", "inf_stamina");
            }

            if (SuperJump)
            {
                Core::g_Scheduler.Execute(R"(
                    local ped = PlayerPedId()
                    if IsControlJustPressed(0, 22) and IsPedOnFoot(ped) then
                        ApplyForceToEntityCenterOfMass(ped, 1, 0.0, 0.0, 18.0, false, false, true, false)
                    end
                )", "super_jump");
            }

            if (SuperRun)
            {
                std::string code = "SetRunSprintMultiplierForPlayer(PlayerId(), " +
                    std::to_string(RunSpeed) + ")";
                Core::g_Scheduler.Execute(code.c_str(), "super_run");
            }

            if (Noclip)
            {
                float speed = NoclipSpeed;
                if (GetAsyncKeyState(VK_SHIFT) & 0x8000) speed *= 3.f;

                std::string code = R"(
                    local ped = PlayerPedId()
                    local camRot = GetGameplayCamRot(2)
                    local dx = 0.0
                    local dy = 0.0
                    local dz = 0.0
                    local speed = )" + std::to_string(speed) + R"(
                    local rad = math.rad

                    FreezeEntityPosition(ped, true)
                    SetEntityCollision(ped, false, false)
                )";

                if (GetAsyncKeyState('W') & 0x8000)
                {
                    code += R"(
                    dy = math.cos(rad(camRot.z)) * speed
                    dx = math.sin(rad(camRot.z)) * speed
                    dz = -math.sin(rad(camRot.x)) * speed
                    )";
                }
                if (GetAsyncKeyState('S') & 0x8000)
                {
                    code += R"(
                    dy = -math.cos(rad(camRot.z)) * speed
                    dx = -math.sin(rad(camRot.z)) * speed
                    dz = math.sin(rad(camRot.x)) * speed
                    )";
                }
                if (GetAsyncKeyState('A') & 0x8000)
                {
                    code += R"(
                    dy = dy + math.cos(rad(camRot.z + 90)) * speed
                    dx = dx + math.sin(rad(camRot.z + 90)) * speed
                    )";
                }
                if (GetAsyncKeyState('D') & 0x8000)
                {
                    code += R"(
                    dy = dy - math.cos(rad(camRot.z + 90)) * speed
                    dx = dx - math.sin(rad(camRot.z + 90)) * speed
                    )";
                }
                if (GetAsyncKeyState(VK_SPACE) & 0x8000)
                    code += "dz = dz + speed\n";
                if (GetAsyncKeyState(VK_CONTROL) & 0x8000)
                    code += "dz = dz - speed\n";

                code += R"(
                    local c = GetEntityCoords(ped)
                    SetEntityCoordsNoOffset(ped, c.x + dx, c.y + dy, c.z + dz, false, false, false)
                    SetEntityHeading(ped, camRot.z)
                end
                )";

                Core::g_Scheduler.Execute(code.c_str(), "noclip");
            }

            if (NightVision)
                Core::g_Scheduler.Execute("SetNightvision(true)", "night_vis");
            else
                Core::g_Scheduler.Execute("SetNightvision(false)", "night_vis_off");

            if (ThermalVision)
                Core::g_Scheduler.Execute("SetSeethrough(true)", "thermal_vis");
            else
                Core::g_Scheduler.Execute("SetSeethrough(false)", "thermal_vis_off");

            if (Blackout)
                Core::g_Scheduler.Execute("SetArtificialLightsState(true)", "blackout");

            if (FastSwim)
                Core::g_Scheduler.Execute("SetSwimMultiplierForPlayer(PlayerId(), 5.0)", "fast_swim");

            if (WantedLevelLock)
            {
                std::string code = "SetPlayerWantedLevel(PlayerId(), " + std::to_string(WantedLevel) + ", false)\n"
                                   "SetPlayerWantedLevelNow(PlayerId(), false)";
                Core::g_Scheduler.Execute(code.c_str(), "wanted_lock");
            }

            if (NoIdleCam)
            {
                Core::g_Scheduler.Execute(R"(
                    InvalidateIdleCam()
                    InvalidateVehicleIdleCam()
                )", "no_idle_cam");
            }

            if (OverrideTime)
            {
                std::string code = "NetworkOverrideClockTime(" + std::to_string(TimeHour) + ", " + std::to_string(TimeMinute) + ", 0)";
                Core::g_Scheduler.Execute(code.c_str(), "override_time");
            }

            if (OverrideWeather && WeatherIndex >= 0 && WeatherIndex < WeatherCount)
            {
                std::string code = std::string("SetWeatherTypeNowPersist('") + WeatherTypes[WeatherIndex] + "')";
                Core::g_Scheduler.Execute(code.c_str(), "override_weather");
            }
        }

        void SetWeather(const char* weather)
        {
            std::string code = "SetWeatherTypeNowPersist('" + std::string(weather) + "')";
            Core::g_Scheduler.ExecuteSafe(code.c_str(), "set_weather");
        }

        void SetTime(int h, int m)
        {
            std::string code = "NetworkOverrideClockTime(" +
                std::to_string(h) + ", " + std::to_string(m) + ", 0)";
            Core::g_Scheduler.ExecuteSafe(code.c_str(), "set_time");
        }

        void ClearArea()
        {
            Core::g_Scheduler.ExecuteSafe(R"(
                local c = GetEntityCoords(PlayerPedId())
                ClearAreaOfEverything(c.x, c.y, c.z, 500.0, false, false, false, false)
            )", "clear_area");
        }

        void SpawnBodyguard()
        {
            Core::g_Scheduler.ExecuteSafe(R"(
                local ped = PlayerPedId()
                local c = GetEntityCoords(ped)
                local model = GetHashKey('s_m_y_blackops_01')
                RequestModel(model)
                while not HasModelLoaded(model) do Wait(100) end
                local guard = CreatePed(4, model, c.x + 2.0, c.y + 2.0, c.z, GetEntityHeading(ped), true, false)
                GiveWeaponToPed(guard, GetHashKey('WEAPON_CARBINERIFLE'), 9999, false, true)
                SetPedRelationshipGroupHash(guard, GetHashKey('PLAYER'))
                TaskCombatHatedTargetsAroundPed(guard, 200.0, 0)
                SetPedKeepTask(guard, true)
                SetModelAsNoLongerNeeded(model)
            )", "spawn_guard");
        }

        void HealPlayer()
        {
            Core::g_Scheduler.ExecuteSafe(R"(
                local ped = PlayerPedId()
                SetEntityHealth(ped, GetEntityMaxHealth(ped))
                ClearPedBloodDamage(ped)
                SetPedArmour(ped, 100)
            )", "heal_player");
        }

        void GiveAllWeapons()
        {
            Core::g_Scheduler.ExecuteSafe(R"(
                local ped = PlayerPedId()
                local weapons = {
                    'WEAPON_PISTOL','WEAPON_COMBATPISTOL','WEAPON_APPISTOL','WEAPON_PISTOL50',
                    'WEAPON_REVOLVER','WEAPON_SNSPISTOL','WEAPON_HEAVYPISTOL','WEAPON_VINTAGEPISTOL',
                    'WEAPON_MICROSMG','WEAPON_SMG','WEAPON_ASSAULTSMG','WEAPON_COMBATPDW','WEAPON_MG','WEAPON_COMBATMG',
                    'WEAPON_ASSAULTRIFLE','WEAPON_CARBINERIFLE','WEAPON_ADVANCEDRIFLE','WEAPON_SPECIALCARBINE','WEAPON_BULLPUPRIFLE',
                    'WEAPON_COMPACTRIFLE','WEAPON_PUMPSHOTGUN','WEAPON_SAWNOFFSHOTGUN','WEAPON_BULLPUPSHOTGUN','WEAPON_ASSAULTSHOTGUN',
                    'WEAPON_HEAVYSHOTGUN','WEAPON_COMBATSHOTGUN','WEAPON_SNIPERRIFLE','WEAPON_HEAVYSNIPER','WEAPON_MARKSMANRIFLE',
                    'WEAPON_RPG','WEAPON_GRENADELAUNCHER','WEAPON_MINIGUN','WEAPON_RAILGUN','WEAPON_HOMINGLAUNCHER',
                    'WEAPON_GRENADE','WEAPON_STICKYBOMB','WEAPON_SMOKEGRE','WEAPON_MOLOTOV','WEAPON_PROXMINE',
                    'WEAPON_KNIFE','WEAPON_BAT','WEAPON_CROWBAR','WEAPON_GOLFCLUB','WEAPON_HAMMER',
                    'WEAPON_HATCHET','WEAPON_MACHETE','WEAPON_SWITCHBLADE','WEAPON_BATTLEAXE',
                    'WEAPON_STUNGUN','WEAPON_FLASHLIGHT','WEAPON_NIGHTSTICK',
                    'WEAPON_PETROLCAN','WEAPON_FIREEXTINGUISHER','WEAPON_PARACHUTE','WEAPON_FLAREGUN'
                }
                for _, w in ipairs(weapons) do
                    GiveWeaponToPed(ped, GetHashKey(w), 9999, false, false)
                end
            )", "give_all_weapons");
        }

        void RemoveAllWeapons()
        {
            Core::g_Scheduler.ExecuteSafe("RemoveAllPedWeapons(PlayerPedId(), true)", "remove_weapons");
        }

        void MaxArmor()
        {
            Core::g_Scheduler.ExecuteSafe("SetPedArmour(PlayerPedId(), 100)", "max_armor");
        }

        void SetPedModel(const char* model)
        {
            std::string code = R"(
                local model = GetHashKey(')" + std::string(model) + R"(')
                RequestModel(model)
                while not HasModelLoaded(model) do Wait(100) end
                SetPlayerModel(PlayerId(), model)
                SetModelAsNoLongerNeeded(model)
            )";
            Core::g_Scheduler.ExecuteSafe(code.c_str(), "set_model");
        }

        void TeleportToWaypoint()
        {
            Core::g_Scheduler.ExecuteSafe(R"(
                local blip = GetFirstBlipInfoId(8)
                if DoesBlipExist(blip) then
                    local c = GetBlipInfoIdCoord(blip)
                    local ped = PlayerPedId()
                    local found, z = false, 0.0
                    for height = 1.0, 1000.0, 25.0 do
                        SetEntityCoordsNoOffset(ped, c.x, c.y, height, false, false, false)
                        Wait(50)
                        found, z = GetGroundZFor_3dCoord(c.x, c.y, height, false)
                        if found then break end
                    end
                    if found then
                        SetEntityCoordsNoOffset(ped, c.x, c.y, z + 1.0, false, false, false)
                    else
                        SetEntityCoordsNoOffset(ped, c.x, c.y, 200.0, false, false, false)
                    end
                end
            )", "tp_waypoint");
        }

    private:
        WorldFeatures() = default;
    };

    inline WorldFeatures& g_WorldFeatures = WorldFeatures::Get();
}
