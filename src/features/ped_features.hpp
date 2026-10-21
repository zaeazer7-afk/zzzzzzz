#pragma once

#include <core/scheduler.hpp>
#include <string>
#include <vector>

namespace Features
{
    class PedFeatures
    {
    public:
        static PedFeatures& Get()
        {
            static PedFeatures inst;
            return inst;
        }

        bool  RiotMode        = false;
        bool  FreezeNPCs      = false;
        bool  TrafficDensityOverride = false;
        float TrafficDensity  = 1.f;
        bool  PedDensityOverride = false;
        float PedDensity      = 1.f;
        bool  EveryoneAttacks = false;
        bool  PedsFollow      = false;

        char  SpawnModelName[64]  = "a_m_m_skater_01";
        char  PlayerModelName[64] = "a_m_m_skater_01";

        const char* AnimalModels[17] = {
            "a_c_boar", "a_c_cat_01", "a_c_chickenhawk", "a_c_chimp",
            "a_c_cow", "a_c_coyote", "a_c_deer", "a_c_dog",
            "a_c_hen", "a_c_husky", "a_c_mtlion", "a_c_pig",
            "a_c_poodle", "a_c_rabbit_01", "a_c_retriever",
            "a_c_rottweiler", "a_c_shepherd"
        };
        int SelectedAnimal = 0;

        void Update()
        {
            if ( RiotMode )
            {
                Core::g_Scheduler.Execute( R"(
                    local rel1 = GetHashKey('RIOT_RELS')
                    for _, ped in ipairs(GetGamePool('CPed')) do
                        if not IsPedAPlayer(ped) then
                            SetPedRelationshipGroupHash(ped, rel1)
                        end
                    end
                    SetRelationshipBetweenGroups(5, rel1, rel1)
                )" );
            }

            if ( FreezeNPCs )
            {
                Core::g_Scheduler.Execute( R"(
                    local myPed = PlayerPedId()
                    local myCoords = GetEntityCoords(myPed)
                    for _, ped in ipairs(GetGamePool('CPed')) do
                        if not IsPedAPlayer(ped) then
                            local c = GetEntityCoords(ped)
                            local dx = c.x - myCoords.x
                            local dy = c.y - myCoords.y
                            if math.sqrt(dx*dx + dy*dy) < 100.0 then
                                FreezeEntityPosition(ped, true)
                            end
                        end
                    end
                )" );
            }

            if ( TrafficDensityOverride )
            {
                std::string code = "SetVehicleDensityMultiplierThisFrame(" + std::to_string( TrafficDensity ) + ")\n"
                    "SetRandomVehicleDensityMultiplierThisFrame(" + std::to_string( TrafficDensity ) + ")\n"
                    "SetParkedVehicleDensityMultiplierThisFrame(" + std::to_string( TrafficDensity ) + ")";
                Core::g_Scheduler.Execute( code );
            }

            if ( PedDensityOverride )
            {
                std::string code = "SetPedDensityMultiplierThisFrame(" + std::to_string( PedDensity ) + ")\n"
                    "SetScenarioPedDensityMultiplierThisFrame(" + std::to_string( PedDensity ) + ", " + std::to_string( PedDensity ) + ")";
                Core::g_Scheduler.Execute( code );
            }

            if ( EveryoneAttacks )
            {
                Core::g_Scheduler.Execute( R"(
                    local myPed = PlayerPedId()
                    local myCoords = GetEntityCoords(myPed)
                    for _, ped in ipairs(GetGamePool('CPed')) do
                        if not IsPedAPlayer(ped) and not IsPedDeadOrDying(ped, true) then
                            local c = GetEntityCoords(ped)
                            local dx = c.x - myCoords.x
                            local dy = c.y - myCoords.y
                            if math.sqrt(dx*dx + dy*dy) < 80.0 then
                                TaskCombatPed(ped, myPed, 0, 16)
                            end
                        end
                    end
                )" );
            }

            if ( PedsFollow )
            {
                Core::g_Scheduler.Execute( R"(
                    local myPed = PlayerPedId()
                    local myCoords = GetEntityCoords(myPed)
                    for _, ped in ipairs(GetGamePool('CPed')) do
                        if not IsPedAPlayer(ped) and not IsPedDeadOrDying(ped, true) then
                            local c = GetEntityCoords(ped)
                            local dx = c.x - myCoords.x
                            local dy = c.y - myCoords.y
                            if math.sqrt(dx*dx + dy*dy) < 60.0 then
                                TaskFollowToOffsetOfEntity(ped, myPed, 0.0, -1.5, 0.0, 5.0, -1, 1.0, true)
                            end
                        end
                    end
                )" );
            }
        }

        void SpawnPed()
        {
            std::string model = SpawnModelName;
            std::string code = R"(
                local hash = GetHashKey(')" + model + R"(')
                RequestModel(hash)
                local t = 0
                while not HasModelLoaded(hash) and t < 50 do
                    Citizen.Wait(100)
                    t = t + 1
                end
                if HasModelLoaded(hash) then
                    local ped = PlayerPedId()
                    local c = GetEntityCoords(ped)
                    local fwd = GetEntityForwardVector(ped)
                    local spawned = CreatePed(4, hash, c.x + fwd.x * 3.0, c.y + fwd.y * 3.0, c.z, GetEntityHeading(ped), true, false)
                    SetModelAsNoLongerNeeded(hash)
                end
            )";
            Core::g_Scheduler.Execute( code );
        }

        void SpawnAnimal()
        {
            std::string model = AnimalModels[SelectedAnimal];
            std::string code = R"(
                local hash = GetHashKey(')" + model + R"(')
                RequestModel(hash)
                local t = 0
                while not HasModelLoaded(hash) and t < 50 do
                    Citizen.Wait(100)
                    t = t + 1
                end
                if HasModelLoaded(hash) then
                    local ped = PlayerPedId()
                    local c = GetEntityCoords(ped)
                    local fwd = GetEntityForwardVector(ped)
                    CreatePed(28, hash, c.x + fwd.x * 3.0, c.y + fwd.y * 3.0, c.z, 0.0, true, false)
                    SetModelAsNoLongerNeeded(hash)
                end
            )";
            Core::g_Scheduler.Execute( code );
        }

        void ClearNearbyPeds( float radius = 50.f )
        {
            std::string code = R"(
                local myPed = PlayerPedId()
                local myCoords = GetEntityCoords(myPed)
                for _, ped in ipairs(GetGamePool('CPed')) do
                    if not IsPedAPlayer(ped) then
                        local c = GetEntityCoords(ped)
                        local dx = c.x - myCoords.x
                        local dy = c.y - myCoords.y
                        if math.sqrt(dx*dx + dy*dy) < )" + std::to_string( radius ) + R"( then
                            SetEntityAsMissionEntity(ped, true, true)
                            DeleteEntity(ped)
                        end
                    end
                end
            )";
            Core::g_Scheduler.Execute( code );
        }

        void ChangePlayerModel()
        {
            std::string model = PlayerModelName;
            std::string code = R"(
                local hash = GetHashKey(')" + model + R"(')
                RequestModel(hash)
                local t = 0
                while not HasModelLoaded(hash) and t < 50 do
                    Citizen.Wait(100)
                    t = t + 1
                end
                if HasModelLoaded(hash) then
                    SetPlayerModel(PlayerId(), hash)
                    SetModelAsNoLongerNeeded(hash)
                end
            )";
            Core::g_Scheduler.Execute( code );
        }

    private:
        PedFeatures() = default;
    };

    inline PedFeatures& g_PedFeatures = PedFeatures::Get();
}
