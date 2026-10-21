#pragma once
#include <core/scheduler.hpp>
#include <Windows.h>
#include <string>

namespace Features
{
    class Combat
    {
    public:
        static Combat& Get() { static Combat inst; return inst; }

        // Toggles
        bool Triggerbot       = false;
        int  TriggerbotKey    = VK_XBUTTON1;
        int  TriggerbotDelay  = 50;       // ms delay before firing

        bool RapidFire        = false;
        bool ExplosiveAmmo    = false;
        bool InfiniteAmmo     = false;
        bool NoRecoil         = false;
        bool NoSpread         = false;
        bool FastReload       = false;
        bool DamageMultiplier = false;
        float DamageMult      = 5.f;
        bool KillAura         = false;
        float KillAuraRange   = 15.f;

        void Update()
        {
            if (InfiniteAmmo)
            {
                Core::g_Scheduler.Execute(R"(
                    local ped = PlayerPedId()
                    local _,wh = GetCurrentPedWeapon(ped, true)
                    if wh ~= 0 then
                        local _,max = GetMaxAmmo(ped, wh)
                        SetPedAmmo(ped, wh, max)
                        MakePedReloadIfEmpty(ped)
                    end
                )", "infinite_ammo");
            }

            if (NoRecoil)
            {
                Core::g_Scheduler.Execute(R"(
                    local ped = PlayerPedId()
                    local _,wh = GetCurrentPedWeapon(ped, true)
                    if wh ~= 0 then
                        SetWeaponAnimationOverride(ped, `Gang1H`)
                    end
                )", "no_recoil");
            }

            if (NoSpread)
            {
                Core::g_Scheduler.Execute(R"(
                    SetPlayerWeaponDamageModifier(PlayerId(), 1.0)
                    SetPedAccuracy(PlayerPedId(), 100)
                )", "no_spread");
            }

            if (ExplosiveAmmo)
            {
                Core::g_Scheduler.Execute(R"(
                    SetExplosiveAmmoThisFrame(PlayerId())
                )", "explosive_ammo");
            }

            if (RapidFire)
            {
                Core::g_Scheduler.Execute(R"(
                    local ped = PlayerPedId()
                    if IsPedShooting(ped) then
                        local _,wh = GetCurrentPedWeapon(ped, true)
                        if wh ~= 0 then
                            local c = GetEntityCoords(ped)
                            local off = GetOffsetFromEntityInWorldCoords(ped, 0.0, 50.0, 0.0)
                            ShootSingleBulletBetweenCoords(c.x, c.y, c.z, off.x, off.y, off.z, 50, true, wh, ped, true, false, 2000.0)
                        end
                    end
                )", "rapid_fire");
            }

            if (FastReload)
            {
                Core::g_Scheduler.Execute(R"(
                    if IsPedReloading(PlayerPedId()) then
                        MakePedReloadIfEmpty(PlayerPedId())
                    end
                )", "fast_reload");
            }

            if (DamageMultiplier)
            {
                std::string code = "SetPlayerWeaponDamageModifier(PlayerId(), " + std::to_string(DamageMult) + ")";
                Core::g_Scheduler.Execute(code.c_str(), "dmg_mult");
            }

            if (Triggerbot)
            {
                bool keyDown = (GetAsyncKeyState(TriggerbotKey) & 0x8000) != 0;
                if (keyDown)
                {
                    Core::g_Scheduler.Execute(R"(
                        local ped = PlayerPedId()
                        local _,entity = GetEntityPlayerIsFreeAimingAt(PlayerId())
                        if entity ~= 0 and IsEntityAPed(entity) and not IsPedDeadOrDying(entity, true) then
                            local _,wh = GetCurrentPedWeapon(ped, true)
                            if wh ~= 0 then
                                SetPedShootsAtCoord(ped, GetEntityCoords(entity).x, GetEntityCoords(entity).y, GetEntityCoords(entity).z, true)
                            end
                        end
                    )", "triggerbot");
                }
            }

            if (KillAura)
            {
                std::string code = R"(
                    local ped = PlayerPedId()
                    local c = GetEntityCoords(ped)
                    for _,id in ipairs(GetActivePlayers()) do
                        if id ~= PlayerId() then
                            local target = GetPlayerPed(id)
                            local tc = GetEntityCoords(target)
                            if #(c - tc) < )" + std::to_string(KillAuraRange) + R"( and not IsPedDeadOrDying(target, true) then
                                local _,wh = GetCurrentPedWeapon(ped, true)
                                if wh ~= 0 then
                                    ShootSingleBulletBetweenCoords(c.x, c.y, c.z+0.5, tc.x, tc.y, tc.z, 250, true, wh, ped, true, false, 2000.0)
                                end
                            end
                        end
                    end
                )";
                Core::g_Scheduler.Execute(code.c_str(), "kill_aura");
            }
        }

    private:
        Combat() = default;
    };

    inline Combat& g_Combat = Combat::Get();
}
