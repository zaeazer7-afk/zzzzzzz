#pragma once
#include <core/scheduler.hpp>
#include <string>

namespace Features
{
    class Recovery
    {
    public:
        static Recovery& Get() { static Recovery inst; return inst; }

        enum Framework { Auto = 0, QBCore = 1, ESX = 2, VRP = 3, QBox = 4, ND = 5 };

        int         ActiveFramework = Auto;
        int         MoneyAmount     = 50000;
        std::string ItemName        = "phone";
        int         ItemCount       = 1;

        void DetectFramework()
        {
            Core::g_Scheduler.ExecuteSafe(R"(
                if GetResourceState('qb-core') == 'started' or GetResourceState('qbx_core') == 'started' then
                    print('FW:QBCORE')
                elseif GetResourceState('es_extended') == 'started' then
                    print('FW:ESX')
                elseif GetResourceState('vrp') == 'started' or GetResourceState('vRP') == 'started' then
                    print('FW:VRP')
                elseif GetResourceState('ND_Core') == 'started' then
                    print('FW:ND')
                else
                    print('FW:UNKNOWN')
                end
            )", "detect_fw");
        }

        void GiveMoney()
        {
            std::string amt = std::to_string(MoneyAmount);
            switch (ActiveFramework)
            {
            case QBCore:
            case QBox:
                Core::g_Scheduler.ExecuteSafe(
                    ("TriggerServerEvent('QBCore:Server:SetMoney', 'cash', " + amt + ")").c_str(), "give_money");
                break;
            case ESX:
                Core::g_Scheduler.ExecuteSafe(
                    ("TriggerServerEvent('esx:setAccountMoney', 'money', " + amt + ")").c_str(), "give_money");
                break;
            case VRP:
                Core::g_Scheduler.ExecuteSafe(
                    ("TriggerServerEvent('vRP:giveMoney', " + amt + ")").c_str(), "give_money");
                break;
            case ND:
                Core::g_Scheduler.ExecuteSafe(
                    ("TriggerServerEvent('ND:money:add', " + amt + ")").c_str(), "give_money");
                break;
            default:
                Core::g_Scheduler.ExecuteSafe(
                    ("TriggerServerEvent('QBCore:Server:SetMoney', 'cash', " + amt + ")\n"
                     "TriggerServerEvent('esx:setAccountMoney', 'money', " + amt + ")").c_str(), "give_money");
                break;
            }
        }

        void GiveBlackMoney()
        {
            std::string amt = std::to_string(MoneyAmount);
            switch (ActiveFramework)
            {
            case QBCore:
            case QBox:
                Core::g_Scheduler.ExecuteSafe(
                    ("TriggerServerEvent('QBCore:Server:SetMoney', 'crypto', " + amt + ")").c_str(), "give_crypto");
                break;
            case ESX:
                Core::g_Scheduler.ExecuteSafe(
                    ("TriggerServerEvent('esx:setAccountMoney', 'black_money', " + amt + ")").c_str(), "give_black");
                break;
            default:
                Core::g_Scheduler.ExecuteSafe(
                    ("TriggerServerEvent('QBCore:Server:SetMoney', 'crypto', " + amt + ")").c_str(), "give_crypto");
                break;
            }
        }

        void GiveItem(const char* item, int count)
        {
            std::string sItem(item);
            std::string sCount = std::to_string(count);
            switch (ActiveFramework)
            {
            case QBCore:
            case QBox:
                Core::g_Scheduler.ExecuteSafe(
                    ("TriggerServerEvent('QBCore:Server:AddItem', '" + sItem + "', " + sCount + ")").c_str(), "give_item");
                break;
            case ESX:
                Core::g_Scheduler.ExecuteSafe(
                    ("TriggerServerEvent('esx:addInventoryItem', '" + sItem + "', " + sCount + ")").c_str(), "give_item");
                break;
            default:
                Core::g_Scheduler.ExecuteSafe(
                    ("TriggerServerEvent('QBCore:Server:AddItem', '" + sItem + "', " + sCount + ")").c_str(), "give_item");
                break;
            }
        }

        void GiveCar(const char* model)
        {
            std::string sModel(model);
            switch (ActiveFramework)
            {
            case QBCore:
            case QBox:
                Core::g_Scheduler.ExecuteSafe(
                    ("TriggerServerEvent('qb-garages:server:addVehicle', '" + sModel + "')").c_str(), "give_car");
                break;
            case ESX:
                Core::g_Scheduler.ExecuteSafe(
                    ("TriggerServerEvent('esx_vehicleshop:buyVehicle', '" + sModel + "')").c_str(), "give_car");
                break;
            default:
                Core::g_Scheduler.ExecuteSafe(
                    ("TriggerServerEvent('qb-garages:server:addVehicle', '" + sModel + "')").c_str(), "give_car");
                break;
            }
        }

        void SetJob(const char* job, int grade)
        {
            std::string sJob(job);
            std::string sGrade = std::to_string(grade);
            switch (ActiveFramework)
            {
            case QBCore:
            case QBox:
                Core::g_Scheduler.ExecuteSafe(
                    ("TriggerServerEvent('QBCore:Server:SetJob', '" + sJob + "', " + sGrade + ")").c_str(), "set_job");
                break;
            case ESX:
                Core::g_Scheduler.ExecuteSafe(
                    ("TriggerServerEvent('esx:setJob', '" + sJob + "', " + sGrade + ")").c_str(), "set_job");
                break;
            default:
                Core::g_Scheduler.ExecuteSafe(
                    ("TriggerServerEvent('QBCore:Server:SetJob', '" + sJob + "', " + sGrade + ")").c_str(), "set_job");
                break;
            }
        }

        void GiveWeaponItem(const char* weapon)
        {
            std::string sWeapon(weapon);
            switch (ActiveFramework)
            {
            case QBCore:
            case QBox:
                Core::g_Scheduler.ExecuteSafe(
                    ("TriggerServerEvent('QBCore:Server:AddItem', '" + sWeapon + "', 1)").c_str(), "give_weapon");
                break;
            case ESX:
                Core::g_Scheduler.ExecuteSafe(
                    ("TriggerServerEvent('esx:addInventoryItem', '" + sWeapon + "', 1)").c_str(), "give_weapon");
                break;
            default:
                Core::g_Scheduler.ExecuteSafe(
                    ("TriggerServerEvent('QBCore:Server:AddItem', '" + sWeapon + "', 1)").c_str(), "give_weapon");
                break;
            }
        }

        void ReviveMe()
        {
            switch (ActiveFramework)
            {
            case QBCore:
            case QBox:
                Core::g_Scheduler.ExecuteSafe("TriggerServerEvent('hospital:server:Revive')", "revive_me");
                break;
            case ESX:
                Core::g_Scheduler.ExecuteSafe("TriggerServerEvent('esx_ambulancejob:revive')", "revive_me");
                break;
            default:
                Core::g_Scheduler.ExecuteSafe(R"(
                    TriggerServerEvent('hospital:server:Revive')
                    TriggerServerEvent('esx_ambulancejob:revive')
                    local ped = PlayerPedId()
                    if IsEntityDead(ped) then
                        local c = GetEntityCoords(ped)
                        NetworkResurrectLocalPlayer(c.x, c.y, c.z, GetEntityHeading(ped), true, false)
                        SetEntityHealth(ped, GetEntityMaxHealth(ped))
                    end
                )", "revive_me");
                break;
            }
        }

        void MaxStats()
        {
            switch (ActiveFramework)
            {
            case QBCore:
            case QBox:
                Core::g_Scheduler.ExecuteSafe(R"(
                    TriggerServerEvent('QBCore:Server:SetMetaData', 'hunger', 100)
                    TriggerServerEvent('QBCore:Server:SetMetaData', 'thirst', 100)
                    TriggerServerEvent('QBCore:Server:SetMetaData', 'stress', 0)
                )", "max_stats");
                break;
            case ESX:
                Core::g_Scheduler.ExecuteSafe(R"(
                    TriggerEvent('esx_status:set', 'hunger', 1000000)
                    TriggerEvent('esx_status:set', 'thirst', 1000000)
                )", "max_stats");
                break;
            default:
                Core::g_Scheduler.ExecuteSafe(R"(
                    TriggerServerEvent('QBCore:Server:SetMetaData', 'hunger', 100)
                    TriggerServerEvent('QBCore:Server:SetMetaData', 'thirst', 100)
                    TriggerServerEvent('QBCore:Server:SetMetaData', 'stress', 0)
                    TriggerEvent('esx_status:set', 'hunger', 1000000)
                    TriggerEvent('esx_status:set', 'thirst', 1000000)
                )", "max_stats");
                break;
            }
        }

    private:
        Recovery() = default;
    };

    inline Recovery& g_Recovery = Recovery::Get();
}
