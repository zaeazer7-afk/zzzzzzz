#pragma once
#include <core/scheduler.hpp>
#include <string>

namespace Features
{
    class Troll
    {
    public:
        static Troll& Get() { static Troll inst; return inst; }

        // One-shot actions (called from menu buttons, not Update loop)

        void ExplodePlayer(int serverId)
        {
            std::string code = R"(
                local target = GetPlayerFromServerId()" + std::to_string(serverId) + R"()
                if target and target ~= -1 then
                    local ped = GetPlayerPed(target)
                    local c = GetEntityCoords(ped)
                    AddExplosion(c.x, c.y, c.z, 2, 100.0, true, false, 1.0)
                end
            )";
            Core::g_Scheduler.ExecuteSafe(code.c_str(), "explode_player");
        }

        void CagePlayer(int serverId)
        {
            std::string code = R"(
                local target = GetPlayerFromServerId()" + std::to_string(serverId) + R"()
                if target and target ~= -1 then
                    local ped = GetPlayerPed(target)
                    local c = GetEntityCoords(ped)
                    local model = GetHashKey('prop_gold_cont_01')
                    RequestModel(model)
                    while not HasModelLoaded(model) do Wait(100) end
                    local obj = CreateObject(model, c.x, c.y, c.z - 1.0, true, true, false)
                    FreezeEntityPosition(obj, true)
                end
            )";
            Core::g_Scheduler.ExecuteSafe(code.c_str(), "cage_player");
        }

        void AttachProp(int serverId, const char* propName = "prop_barrel_02a")
        {
            std::string code = R"(
                local target = GetPlayerFromServerId()" + std::to_string(serverId) + R"()
                if target and target ~= -1 then
                    local ped = GetPlayerPed(target)
                    local model = GetHashKey(')" + std::string(propName) + R"(')
                    RequestModel(model)
                    while not HasModelLoaded(model) do Wait(100) end
                    local obj = CreateObject(model, 0, 0, 0, true, true, false)
                    AttachEntityToEntity(obj, ped, GetPedBoneIndex(ped, 0), 0, 0, 0.5, 0, 0, 0, true, true, false, true, 1, true)
                end
            )";
            Core::g_Scheduler.ExecuteSafe(code.c_str(), "attach_prop");
        }

        void BringPlayer(int serverId)
        {
            std::string code = R"(
                local target = GetPlayerFromServerId()" + std::to_string(serverId) + R"()
                if target and target ~= -1 then
                    local ped = GetPlayerPed(target)
                    local c = GetEntityCoords(PlayerPedId())
                    SetEntityCoords(ped, c.x, c.y, c.z, false, false, false, false)
                end
            )";
            Core::g_Scheduler.ExecuteSafe(code.c_str(), "bring_player");
        }

        void TPToPlayer(int serverId)
        {
            std::string code = R"(
                local target = GetPlayerFromServerId()" + std::to_string(serverId) + R"()
                if target and target ~= -1 then
                    local ped = GetPlayerPed(target)
                    local c = GetEntityCoords(ped)
                    SetEntityCoordsNoOffset(PlayerPedId(), c.x, c.y, c.z, false, false, false)
                end
            )";
            Core::g_Scheduler.ExecuteSafe(code.c_str(), "tp_to_player");
        }

        void SpectatePlayer(int serverId)
        {
            std::string code = R"(
                local target = GetPlayerFromServerId()" + std::to_string(serverId) + R"()
                if target and target ~= -1 then
                    local ped = GetPlayerPed(target)
                    NetworkSetInSpectatorMode(true, ped)
                end
            )";
            Core::g_Scheduler.ExecuteSafe(code.c_str(), "spectate_player");
        }

        void StopSpectate()
        {
            Core::g_Scheduler.ExecuteSafe("NetworkSetInSpectatorMode(false, PlayerPedId())", "stop_spectate");
        }

        void ClonePlayer(int serverId)
        {
            std::string code = R"(
                local target = GetPlayerFromServerId()" + std::to_string(serverId) + R"()
                if target and target ~= -1 then
                    local ped = GetPlayerPed(target)
                    local model = GetEntityModel(ped)
                    RequestModel(model)
                    while not HasModelLoaded(model) do Wait(100) end
                    local c = GetEntityCoords(ped)
                    local clone = CreatePed(4, model, c.x+1.0, c.y, c.z, GetEntityHeading(ped), true, false)
                    SetModelAsNoLongerNeeded(model)
                end
            )";
            Core::g_Scheduler.ExecuteSafe(code.c_str(), "clone_player");
        }

        void ForceRagdoll(int serverId)
        {
            std::string code = R"(
                local target = GetPlayerFromServerId()" + std::to_string(serverId) + R"()
                if target and target ~= -1 then
                    local ped = GetPlayerPed(target)
                    SetPedToRagdoll(ped, 5000, 5000, 0, false, false, false)
                end
            )";
            Core::g_Scheduler.ExecuteSafe(code.c_str(), "ragdoll_player");
        }

        void KillNearbyPeds()
        {
            Core::g_Scheduler.ExecuteSafe(R"(
                local me = PlayerPedId()
                local handle, ped = FindFirstPed()
                local found = true
                while found do
                    if ped ~= me and not IsPedAPlayer(ped) then
                        SetEntityHealth(ped, 0)
                    end
                    found, ped = FindNextPed(handle)
                end
                EndFindPed(handle)
            )", "kill_peds");
        }

        void ExplodeAllVehicles()
        {
            Core::g_Scheduler.ExecuteSafe(R"(
                local handle, veh = FindFirstVehicle()
                local found = true
                while found do
                    if not IsPedInVehicle(PlayerPedId(), veh, false) then
                        NetworkExplodeVehicle(veh, true, true, false)
                    end
                    found, veh = FindNextVehicle(handle)
                end
                EndFindVehicle(handle)
            )", "explode_vehicles");
        }

        void MoneyDrop(int serverId)
        {
            std::string code = R"(
                local target = GetPlayerFromServerId()" + std::to_string(serverId) + R"()
                if target and target ~= -1 then
                    local ped = GetPlayerPed(target)
                    local c = GetEntityCoords(ped)
                    local model = GetHashKey('prop_money_bag_01')
                    RequestModel(model)
                    while not HasModelLoaded(model) do Wait(100) end
                    for i = 1, 20 do
                        local obj = CreateObject(model, c.x + math.random(-2,2), c.y + math.random(-2,2), c.z + 3.0 + i*0.3, true, true, false)
                    end
                    SetModelAsNoLongerNeeded(model)
                end
            )";
            Core::g_Scheduler.ExecuteSafe(code.c_str(), "money_drop");
        }

        void SoundSpam(int serverId)
        {
            std::string code = R"(
                local target = GetPlayerFromServerId()" + std::to_string(serverId) + R"()
                if target and target ~= -1 then
                    local ped = GetPlayerPed(target)
                    local c = GetEntityCoords(ped)
                    PlaySoundFromCoord(-1, "TIMER_STOP", c.x, c.y, c.z, "HUD_MINI_GAME_SOUNDSET", false, 100, true)
                end
            )";
            Core::g_Scheduler.ExecuteSafe(code.c_str(), "sound_spam");
        }

        void FakeMessage(const char* msg)
        {
            std::string code = "TriggerEvent('chat:addMessage', {args = {'SYSTEM', '";
            code += msg;
            code += "'}})";
            Core::g_Scheduler.ExecuteSafe(code.c_str(), "fake_msg");
        }

    private:
        Troll() = default;
    };

    inline Troll& g_Troll = Troll::Get();
}
