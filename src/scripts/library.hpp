#pragma once

#include <imgui/imgui.h>
#include <string>
#include <vector>
#include <unordered_map>
#include <algorithm>

namespace Scripts
{
    struct ScriptParam
    {
        std::string name;
        std::string defaultValue;
        std::string description;
    };

    struct Script
    {
        std::string            category;
        std::string            name;
        std::string            description;
        std::string            code;
        std::vector<ScriptParam> params;
        ImVec4                 categoryColor;
    };

    class Library
    {
    public:
        static Library& Get()
        {
            static Library inst;
            return inst;
        }

        const std::vector<Script>& GetAll() const { return m_scripts; }

        std::vector<std::string> GetCategories() const
        {
            std::vector<std::string> cats;
            for ( auto& s : m_scripts )
            {
                if ( std::find( cats.begin(), cats.end(), s.category ) == cats.end() )
                    cats.push_back( s.category );
            }
            return cats;
        }

        std::vector<const Script*> GetByCategory( const std::string& cat ) const
        {
            std::vector<const Script*> result;
            for ( auto& s : m_scripts )
                if ( s.category == cat )
                    result.push_back( &s );
            return result;
        }

        std::vector<const Script*> Search( const std::string& query ) const
        {
            std::vector<const Script*> result;
            std::string q = query;
            std::transform( q.begin(), q.end(), q.begin(), ::tolower );
            for ( auto& s : m_scripts )
            {
                std::string n = s.name, d = s.description;
                std::transform( n.begin(), n.end(), n.begin(), ::tolower );
                std::transform( d.begin(), d.end(), d.begin(), ::tolower );
                if ( n.find( q ) != std::string::npos || d.find( q ) != std::string::npos )
                    result.push_back( &s );
            }
            return result;
        }

        static std::string BuildCode( const Script& script, const std::unordered_map<std::string, std::string>& paramValues )
        {
            std::string code = script.code;
            for ( auto& [name, val] : paramValues )
            {
                std::string placeholder = "%" + name + "%";
                size_t pos;
                while ( ( pos = code.find( placeholder ) ) != std::string::npos )
                    code.replace( pos, placeholder.length(), val );
            }
            return code;
        }

        ImVec4 GetCategoryColor( const std::string& cat ) const
        {
            for ( auto& s : m_scripts )
                if ( s.category == cat )
                    return s.categoryColor;
            return ImVec4( 0.7f, 0.7f, 0.7f, 1.f );
        }

    private:
        Library()
        {
            ImVec4 cQB   = ImVec4( 0.2f, 0.9f, 0.3f, 1.f );
            ImVec4 cESX  = ImVec4( 0.3f, 0.5f, 1.f, 1.f );
            ImVec4 cQBOX = ImVec4( 1.f, 0.6f, 0.2f, 1.f );
            ImVec4 cVRP  = ImVec4( 1.f, 0.3f, 0.3f, 1.f );
            ImVec4 cND   = ImVec4( 0.7f, 0.3f, 1.f, 1.f );
            ImVec4 cVeh  = ImVec4( 0.2f, 0.8f, 0.8f, 1.f );
            ImVec4 cWpn  = ImVec4( 1.f, 0.9f, 0.2f, 1.f );
            ImVec4 cPlr  = ImVec4( 0.2f, 0.9f, 0.9f, 1.f );
            ImVec4 cWld  = ImVec4( 0.9f, 0.3f, 0.9f, 1.f );
            ImVec4 cTrl  = ImVec4( 1.f, 0.2f, 0.2f, 1.f );
            ImVec4 cUtl  = ImVec4( 0.6f, 0.6f, 0.6f, 1.f );
            ImVec4 cAdm  = ImVec4( 0.7f, 0.1f, 0.1f, 1.f );
            ImVec4 cRec  = ImVec4( 1.f, 0.8f, 0.2f, 1.f );

            // ── QBCore ──
            m_scripts.push_back( { "QBCore", "Give Cash", "Add cash to your account",
                "TriggerServerEvent('QBCore:Server:SetMoney', 'cash', %AMOUNT%)", { { "AMOUNT", "50000", "Amount of cash" } }, cQB } );
            m_scripts.push_back( { "QBCore", "Give Bank", "Add money to bank",
                "TriggerServerEvent('QBCore:Server:SetMoney', 'bank', %AMOUNT%)", { { "AMOUNT", "100000", "Amount" } }, cQB } );
            m_scripts.push_back( { "QBCore", "Give Crypto", "Add crypto currency",
                "TriggerServerEvent('QBCore:Server:SetMoney', 'crypto', %AMOUNT%)", { { "AMOUNT", "500", "Amount" } }, cQB } );
            m_scripts.push_back( { "QBCore", "Give Item", "Add item to inventory",
                "TriggerServerEvent('QBCore:Server:AddItem', '%ITEM%', %COUNT%)", { { "ITEM", "lockpick", "Item name" }, { "COUNT", "10", "Quantity" } }, cQB } );
            m_scripts.push_back( { "QBCore", "Set Job", "Change your job",
                "TriggerServerEvent('QBCore:Server:SetJob', '%JOB%', %GRADE%)", { { "JOB", "police", "Job name" }, { "GRADE", "4", "Grade level" } }, cQB } );
            m_scripts.push_back( { "QBCore", "Revive Self", "Revive yourself",
                "TriggerServerEvent('hospital:server:RespawnPlayer')\nTriggerEvent('hospital:client:Revive')", {}, cQB } );
            m_scripts.push_back( { "QBCore", "Uncuff Self", "Remove handcuffs",
                "TriggerEvent('police:client:GetCuffed', false)\nTriggerEvent('hospital:client:Revive')", {}, cQB } );
            m_scripts.push_back( { "QBCore", "Open Trunk", "Put player in trunk",
                "TriggerServerEvent('police:server:PutInTrunk', %TARGET_ID%)", { { "TARGET_ID", "1", "Target server ID" } }, cQB } );
            m_scripts.push_back( { "QBCore", "Give All Weapons", "Give all weapons via QB",
                "TriggerServerEvent('QBCore:Server:AddItem', 'weapon_pistol', 1)\nTriggerServerEvent('QBCore:Server:AddItem', 'weapon_smg', 1)\nTriggerServerEvent('QBCore:Server:AddItem', 'weapon_assaultrifle', 1)\nTriggerServerEvent('QBCore:Server:AddItem', 'weapon_sniperrifle', 1)\nTriggerServerEvent('QBCore:Server:AddItem', 'weapon_pumpshotgun', 1)", {}, cQB } );
            m_scripts.push_back( { "QBCore", "Set Gang", "Change your gang",
                "TriggerServerEvent('QBCore:Server:SetGang', '%GANG%', %GRADE%)", { { "GANG", "ballas", "Gang name" }, { "GRADE", "3", "Grade" } }, cQB } );
            m_scripts.push_back( { "QBCore", "Clear Inventory", "Wipe inventory",
                "TriggerServerEvent('QBCore:Server:ClearInventory')", {}, cQB } );
            m_scripts.push_back( { "QBCore", "Max Stats", "Max all character stats",
                "local p = PlayerPedId()\nSetEntityHealth(p, 200)\nSetPedArmour(p, 100)\nRestorePlayerStamina(PlayerId(), 1.0)\nStatSetInt(GetHashKey('MP0_STAMINA'), 100, true)\nStatSetInt(GetHashKey('MP0_STRENGTH'), 100, true)\nStatSetInt(GetHashKey('MP0_LUNG_CAPACITY'), 100, true)\nStatSetInt(GetHashKey('MP0_SHOOTING_ABILITY'), 100, true)\nStatSetInt(GetHashKey('MP0_STEALTH_ABILITY'), 100, true)", {}, cQB } );

            // ── ESX ──
            m_scripts.push_back( { "ESX", "Give Money", "Add money via ESX",
                "TriggerServerEvent('esx:giveMoney', %AMOUNT%)", { { "AMOUNT", "50000", "Amount" } }, cESX } );
            m_scripts.push_back( { "ESX", "Give Item", "Add ESX item",
                "TriggerServerEvent('esx:giveInventoryItem', '%ITEM%', 'item_standard', %COUNT%)", { { "ITEM", "bread", "Item" }, { "COUNT", "10", "Quantity" } }, cESX } );
            m_scripts.push_back( { "ESX", "Give Weapon", "Give weapon via ESX",
                "TriggerServerEvent('esx:giveWeapon', '%WEAPON%', 500)", { { "WEAPON", "WEAPON_PISTOL", "Weapon hash" } }, cESX } );
            m_scripts.push_back( { "ESX", "Set Job", "Set ESX job",
                "TriggerServerEvent('esx:setJob', '%JOB%', %GRADE%)", { { "JOB", "police", "Job" }, { "GRADE", "4", "Grade" } }, cESX } );
            m_scripts.push_back( { "ESX", "Revive", "Revive via ESX",
                "TriggerServerEvent('esx_ambulancejob:revive')\nlocal p = PlayerPedId()\nSetEntityHealth(p, 200)", {}, cESX } );
            m_scripts.push_back( { "ESX", "Heal", "Heal via ESX",
                "local p = PlayerPedId()\nSetEntityHealth(p, 200)\nSetPedArmour(p, 100)", {}, cESX } );
            m_scripts.push_back( { "ESX", "Give Car", "Spawn car via ESX",
                "TriggerServerEvent('esx_vehicleshop:buyVehicle', '%MODEL%')", { { "MODEL", "adder", "Vehicle model" } }, cESX } );
            m_scripts.push_back( { "ESX", "Set Admin", "Set admin level",
                "TriggerServerEvent('esx:setAccountMoney', 'admin', %LEVEL%)", { { "LEVEL", "99", "Level" } }, cESX } );

            // ── QBOX ──
            m_scripts.push_back( { "QBOX", "Give Money", "QBOX framework money",
                "TriggerServerEvent('qbx:server:giveMoney', '%TYPE%', %AMOUNT%)", { { "TYPE", "cash", "cash/bank/crypto" }, { "AMOUNT", "50000", "Amount" } }, cQBOX } );
            m_scripts.push_back( { "QBOX", "Give Item", "QBOX add item",
                "TriggerServerEvent('qbx:server:addItem', '%ITEM%', %COUNT%)", { { "ITEM", "lockpick", "Item" }, { "COUNT", "10", "Count" } }, cQBOX } );
            m_scripts.push_back( { "QBOX", "Set Job", "QBOX set job",
                "TriggerServerEvent('qbx:server:setJob', '%JOB%', %GRADE%)", { { "JOB", "police", "Job" }, { "GRADE", "4", "Grade" } }, cQBOX } );
            m_scripts.push_back( { "QBOX", "Revive", "QBOX revive",
                "TriggerServerEvent('qbx:server:revivePlayer')", {}, cQBOX } );
            m_scripts.push_back( { "QBOX", "Set Gang", "QBOX set gang",
                "TriggerServerEvent('qbx:server:setGang', '%GANG%', %GRADE%)", { { "GANG", "ballas", "Gang" }, { "GRADE", "2", "Grade" } }, cQBOX } );

            // ── vRP ──
            m_scripts.push_back( { "vRP", "Give Money", "vRP give money",
                "TriggerServerEvent('vRP:giveMoney', %AMOUNT%)", { { "AMOUNT", "50000", "Amount" } }, cVRP } );
            m_scripts.push_back( { "vRP", "Give Item", "vRP give item",
                "TriggerServerEvent('vRP:giveInventoryItem', '%ITEM%', %COUNT%)", { { "ITEM", "bread", "Item" }, { "COUNT", "10", "Count" } }, cVRP } );
            m_scripts.push_back( { "vRP", "Set Group", "vRP set group",
                "TriggerServerEvent('vRP:setGroup', '%GROUP%')", { { "GROUP", "admin", "Group name" } }, cVRP } );
            m_scripts.push_back( { "vRP", "Revive", "vRP revive",
                "TriggerServerEvent('vRP:revive')\nlocal p = PlayerPedId()\nSetEntityHealth(p, 200)", {}, cVRP } );
            m_scripts.push_back( { "vRP", "TP to Player", "Teleport to vRP player",
                "TriggerServerEvent('vRP:teleportToPlayer', %TARGET_ID%)", { { "TARGET_ID", "1", "Player source ID" } }, cVRP } );

            // ── ND ──
            m_scripts.push_back( { "ND", "Give Money", "ND framework money",
                "TriggerServerEvent('ND:giveMoney', %AMOUNT%)", { { "AMOUNT", "50000", "Amount" } }, cND } );
            m_scripts.push_back( { "ND", "Give Item", "ND give item",
                "TriggerServerEvent('ND:giveItem', '%ITEM%', %COUNT%)", { { "ITEM", "lockpick", "Item" }, { "COUNT", "10", "Count" } }, cND } );
            m_scripts.push_back( { "ND", "Set Job", "ND set job",
                "TriggerServerEvent('ND:setJob', '%JOB%', %GRADE%)", { { "JOB", "police", "Job" }, { "GRADE", "4", "Grade" } }, cND } );

            // ── Vehicle ──
            m_scripts.push_back( { "Vehicle", "Spawn Vehicle", "Spawn any vehicle",
                "local hash = GetHashKey('%MODEL%')\nRequestModel(hash)\nlocal t=0\nwhile not HasModelLoaded(hash) and t<50 do Citizen.Wait(100) t=t+1 end\nif HasModelLoaded(hash) then\n  local p = PlayerPedId()\n  local c = GetEntityCoords(p)\n  local v = CreateVehicle(hash, c.x, c.y, c.z+1.0, GetEntityHeading(p), true, false)\n  SetPedIntoVehicle(p, v, -1)\n  SetVehicleNumberPlateText(v, 'TRIDENT')\n  SetModelAsNoLongerNeeded(hash)\nend",
                { { "MODEL", "adder", "Vehicle model name" } }, cVeh } );
            m_scripts.push_back( { "Vehicle", "Delete Vehicle", "Delete current vehicle",
                "local veh = GetVehiclePedIsIn(PlayerPedId(), false)\nif veh ~= 0 then\n  SetEntityAsMissionEntity(veh, true, true)\n  DeleteVehicle(veh)\nend", {}, cVeh } );
            m_scripts.push_back( { "Vehicle", "Repair Vehicle", "Fix current vehicle",
                "local veh = GetVehiclePedIsIn(PlayerPedId(), false)\nif veh ~= 0 then\n  SetVehicleFixed(veh)\n  SetVehicleDeformationFixed(veh)\n  SetVehicleEngineHealth(veh, 1000.0)\n  SetVehicleBodyHealth(veh, 1000.0)\nend", {}, cVeh } );
            m_scripts.push_back( { "Vehicle", "Max Upgrade", "Fully upgrade vehicle",
                "local veh = GetVehiclePedIsIn(PlayerPedId(), false)\nif veh ~= 0 then\n  SetVehicleModKit(veh, 0)\n  for i=0,49 do\n    local max = GetNumVehicleMods(veh, i)\n    SetVehicleMod(veh, i, max-1, false)\n  end\n  ToggleVehicleMod(veh, 18, true)\n  ToggleVehicleMod(veh, 20, true)\n  ToggleVehicleMod(veh, 22, true)\n  SetVehicleWindowTint(veh, 1)\nend", {}, cVeh } );
            m_scripts.push_back( { "Vehicle", "Vehicle God Mode", "Toggle vehicle invincibility",
                "local veh = GetVehiclePedIsIn(PlayerPedId(), false)\nif veh ~= 0 then\n  SetEntityInvincible(veh, true)\n  SetEntityProofs(veh, true, true, true, true, true, true, true, true)\n  SetVehicleCanBeVisiblyDamaged(veh, false)\nend", {}, cVeh } );
            m_scripts.push_back( { "Vehicle", "Speed Boost", "Boost vehicle speed",
                "local veh = GetVehiclePedIsIn(PlayerPedId(), false)\nif veh ~= 0 then\n  local fwd = GetEntityForwardVector(veh)\n  local boost = %BOOST%\n  SetVehicleForwardSpeed(veh, GetEntitySpeed(veh) + boost)\nend",
                { { "BOOST", "30.0", "Speed boost amount" } }, cVeh } );
            m_scripts.push_back( { "Vehicle", "Flip Vehicle", "Flip vehicle upright",
                "local veh = GetVehiclePedIsIn(PlayerPedId(), false)\nif veh ~= 0 then\n  local c = GetEntityCoords(veh)\n  SetEntityCoords(veh, c.x, c.y, c.z+1.0, false, false, false, false)\n  SetEntityHeading(veh, GetEntityHeading(veh))\n  SetVehicleOnGroundProperly(veh)\nend", {}, cVeh } );
            m_scripts.push_back( { "Vehicle", "Change Plate", "Change license plate",
                "local veh = GetVehiclePedIsIn(PlayerPedId(), false)\nif veh ~= 0 then\n  SetVehicleNumberPlateText(veh, '%PLATE%')\nend",
                { { "PLATE", "TRIDENT", "Plate text (max 8)" } }, cVeh } );
            m_scripts.push_back( { "Vehicle", "Rainbow Color", "Cycle vehicle colors",
                "local veh = GetVehiclePedIsIn(PlayerPedId(), false)\nif veh ~= 0 then\n  local r = math.random(0, 255)\n  local g = math.random(0, 255)\n  local b = math.random(0, 255)\n  SetVehicleCustomPrimaryColour(veh, r, g, b)\n  SetVehicleCustomSecondaryColour(veh, r, g, b)\nend", {}, cVeh } );

            // ── Weapon ──
            m_scripts.push_back( { "Weapon", "Give All Weapons", "Give every weapon",
                "local p = PlayerPedId()\nlocal weapons = {0x1B06D571,0x5EF9FEC4,0xBFE256D4,0x22D8FE39,0x3656C8C1,0x99AEEB3B,0x13532244,0x2BE6766B,0xEFE7E2DF,0xA3D4D34,0xDB1AA450,0xBD248B55,0x476BF155,0x394F415C,0x83BF0278,0xFAD1F1C9,0xAF113F99,0xC0A3098D,0x969C3D67,0x7F229F94,0x84D6FAFD,0x624FE830,0x9D61E50F,0x05FC3C11,0x0C472FE2,0xA914799,0xC734385A,0x6A6C02E0,0x6D544C99}\nfor _,w in ipairs(weapons) do\n  GiveWeaponToPed(p, w, 9999, false, false)\nend", {}, cWpn } );
            m_scripts.push_back( { "Weapon", "Give Weapon", "Give specific weapon",
                "GiveWeaponToPed(PlayerPedId(), GetHashKey('%WEAPON%'), 9999, false, false)",
                { { "WEAPON", "WEAPON_PISTOL", "Weapon name" } }, cWpn } );
            m_scripts.push_back( { "Weapon", "Remove All Weapons", "Strip all weapons",
                "RemoveAllPedWeapons(PlayerPedId(), true)", {}, cWpn } );
            m_scripts.push_back( { "Weapon", "Max Ammo", "Max ammo for all weapons",
                "local p = PlayerPedId()\nlocal weapons = {0x1B06D571,0x5EF9FEC4,0xBFE256D4,0x22D8FE39,0x3656C8C1,0x99AEEB3B,0x13532244,0x2BE6766B,0xEFE7E2DF,0xBD248B55,0x476BF155,0x394F415C,0x83BF0278,0xFAD1F1C9,0xAF113F99,0xC0A3098D,0x969C3D67,0x7F229F94,0x84D6FAFD,0x624FE830}\nfor _,w in ipairs(weapons) do\n  if HasPedGotWeapon(p, w, false) then\n    SetPedAmmo(p, w, 9999)\n  end\nend", {}, cWpn } );
            m_scripts.push_back( { "Weapon", "Explosive Ammo", "Toggle explosive ammo",
                "SetExplosiveAmmoThisFrame(PlayerId())", {}, cWpn } );
            m_scripts.push_back( { "Weapon", "Fire Ammo", "Toggle fire ammo",
                "SetFireAmmoThisFrame(PlayerId())", {}, cWpn } );

            // ── Player ──
            m_scripts.push_back( { "Player", "God Mode", "Toggle invincibility",
                "local p = PlayerPedId()\nSetEntityInvincible(p, true)\nSetEntityProofs(p, true, true, true, true, true, true, true, true)", {}, cPlr } );
            m_scripts.push_back( { "Player", "Heal Full", "Full health + armor",
                "local p = PlayerPedId()\nSetEntityHealth(p, 200)\nSetPedArmour(p, 100)\nClearPedBloodDamage(p)\nResetPedVisibleDamage(p)", {}, cPlr } );
            m_scripts.push_back( { "Player", "Invisible", "Toggle visibility",
                "local p = PlayerPedId()\nSetEntityVisible(p, false, false)\nNetworkFadeOutEntity(p, true, false)", {}, cPlr } );
            m_scripts.push_back( { "Player", "No Ragdoll", "Disable ragdoll",
                "SetPedCanRagdoll(PlayerPedId(), false)\nSetPedCanRagdollFromPlayerImpact(PlayerPedId(), false)", {}, cPlr } );
            m_scripts.push_back( { "Player", "TP to Waypoint", "Teleport to map waypoint",
                "local wp = GetFirstBlipInfoId(8)\nif DoesBlipExist(wp) then\n  local c = GetBlipInfoIdCoord(wp)\n  local z = 0.0\n  local found = false\n  for h=1.0,1000.0,25.0 do\n    SetEntityCoordsNoOffset(PlayerPedId(), c.x, c.y, h, false, false, false)\n    Citizen.Wait(50)\n    local ok, gz = GetGroundZFor_3dCoord(c.x, c.y, h, false)\n    if ok then z = gz found = true break end\n  end\n  if found then\n    SetEntityCoords(PlayerPedId(), c.x, c.y, z+1.0, false, false, false, false)\n  end\nend", {}, cPlr } );
            m_scripts.push_back( { "Player", "TP to Coords", "Teleport to coordinates",
                "SetEntityCoords(PlayerPedId(), %X%, %Y%, %Z%, false, false, false, false)",
                { { "X", "0.0", "X" }, { "Y", "0.0", "Y" }, { "Z", "75.0", "Z" } }, cPlr } );
            m_scripts.push_back( { "Player", "Noclip Toggle", "Toggle noclip flight",
                "local p = PlayerPedId()\nSetEntityCollision(p, false, false)\nFreezeEntityPosition(p, false)", {}, cPlr } );
            m_scripts.push_back( { "Player", "Change Model", "Change player model",
                "local hash = GetHashKey('%MODEL%')\nRequestModel(hash)\nlocal t=0\nwhile not HasModelLoaded(hash) and t<50 do Citizen.Wait(100) t=t+1 end\nif HasModelLoaded(hash) then SetPlayerModel(PlayerId(), hash) SetModelAsNoLongerNeeded(hash) end",
                { { "MODEL", "a_m_m_skater_01", "Model name" } }, cPlr } );
            m_scripts.push_back( { "Player", "Clear Wanted", "Remove wanted level",
                "ClearPlayerWantedLevel(PlayerId())\nSetPlayerWantedLevel(PlayerId(), 0, false)\nSetPlayerWantedLevelNow(PlayerId(), false)", {}, cPlr } );
            m_scripts.push_back( { "Player", "Set Wanted", "Set wanted stars",
                "SetPlayerWantedLevel(PlayerId(), %LEVEL%, false)\nSetPlayerWantedLevelNow(PlayerId(), false)",
                { { "LEVEL", "5", "Stars 0-5" } }, cPlr } );
            m_scripts.push_back( { "Player", "Fast Run", "Super sprint speed",
                "SetRunSprintMultiplierForPlayer(PlayerId(), %SPEED%)",
                { { "SPEED", "3.0", "Multiplier" } }, cPlr } );
            m_scripts.push_back( { "Player", "Super Jump", "Enhanced jump height",
                "SetSuperJumpThisFrame(PlayerId())", {}, cPlr } );
            m_scripts.push_back( { "Player", "Night Vision", "Toggle NV",
                "SetNightvision(not IsNightvisionActive())", {}, cPlr } );
            m_scripts.push_back( { "Player", "Thermal Vision", "Toggle thermal",
                "SetSeethrough(not IsSeethroughActive())", {}, cPlr } );

            // ── World ──
            m_scripts.push_back( { "World", "Set Time", "Change world time",
                "NetworkOverrideClockTime(%HOUR%, %MINUTE%, 0)",
                { { "HOUR", "12", "Hour 0-23" }, { "MINUTE", "0", "Minute 0-59" } }, cWld } );
            m_scripts.push_back( { "World", "Set Weather", "Change weather",
                "SetWeatherTypeNowPersist('%WEATHER%')",
                { { "WEATHER", "CLEAR", "Weather type" } }, cWld } );
            m_scripts.push_back( { "World", "Blackout", "Toggle blackout",
                "SetArtificialLightsState(true)", {}, cWld } );
            m_scripts.push_back( { "World", "Spawn Object", "Spawn a prop",
                "local hash = GetHashKey('%MODEL%')\nRequestModel(hash)\nlocal t=0\nwhile not HasModelLoaded(hash) and t<50 do Citizen.Wait(100) t=t+1 end\nif HasModelLoaded(hash) then\n  local c = GetEntityCoords(PlayerPedId())\n  local fwd = GetEntityForwardVector(PlayerPedId())\n  CreateObject(hash, c.x+fwd.x*3.0, c.y+fwd.y*3.0, c.z, true, false, false)\n  SetModelAsNoLongerNeeded(hash)\nend",
                { { "MODEL", "prop_barrel_02a", "Prop model" } }, cWld } );
            m_scripts.push_back( { "World", "Delete Nearby Objects", "Remove props nearby",
                "local c = GetEntityCoords(PlayerPedId())\nfor _,obj in ipairs(GetGamePool('CObject')) do\n  local oc = GetEntityCoords(obj)\n  local d = #(c - oc)\n  if d < 30.0 then\n    SetEntityAsMissionEntity(obj, true, true)\n    DeleteEntity(obj)\n  end\nend", {}, cWld } );
            m_scripts.push_back( { "World", "Explosion", "Create explosion",
                "AddExplosion(%X%, %Y%, %Z%, %TYPE%, 100.0, true, false, 1.0)",
                { { "X", "0.0", "X" }, { "Y", "0.0", "Y" }, { "Z", "75.0", "Z" }, { "TYPE", "2", "Explosion type 0-82" } }, cWld } );

            // ── Troll ──
            m_scripts.push_back( { "Troll", "Explode Player", "Blow up target player",
                "local target = GetPlayerPed(GetPlayerFromServerId(%TARGET_ID%))\nif target ~= 0 then\n  local c = GetEntityCoords(target)\n  AddExplosion(c.x, c.y, c.z, 2, 100.0, true, false, 1.0)\nend",
                { { "TARGET_ID", "1", "Server ID" } }, cTrl } );
            m_scripts.push_back( { "Troll", "Cage Player", "Trap in cage",
                "local target = GetPlayerPed(GetPlayerFromServerId(%TARGET_ID%))\nif target ~= 0 then\n  local c = GetEntityCoords(target)\n  local hash = GetHashKey('prop_gold_cont_01')\n  RequestModel(hash)\n  while not HasModelLoaded(hash) do Citizen.Wait(10) end\n  local cage = CreateObject(hash, c.x, c.y, c.z-1.0, true, false, false)\n  FreezeEntityPosition(cage, true)\nend",
                { { "TARGET_ID", "1", "Server ID" } }, cTrl } );
            m_scripts.push_back( { "Troll", "Attach Prop", "Attach prop to player",
                "local target = GetPlayerPed(GetPlayerFromServerId(%TARGET_ID%))\nif target ~= 0 then\n  local hash = GetHashKey('%PROP%')\n  RequestModel(hash)\n  while not HasModelLoaded(hash) do Citizen.Wait(10) end\n  local obj = CreateObject(hash, 0, 0, 0, true, false, false)\n  AttachEntityToEntity(obj, target, GetPedBoneIndex(target, 0x796E), 0.0, 0.0, 0.5, 0.0, 0.0, 0.0, true, true, false, true, 1, true)\nend",
                { { "TARGET_ID", "1", "Server ID" }, { "PROP", "prop_cs_dildo_01", "Prop model" } }, cTrl } );
            m_scripts.push_back( { "Troll", "Bring Player", "Teleport player to you",
                "local me = GetEntityCoords(PlayerPedId())\nlocal target = GetPlayerPed(GetPlayerFromServerId(%TARGET_ID%))\nif target ~= 0 then\n  SetEntityCoords(target, me.x, me.y, me.z, false, false, false, false)\nend",
                { { "TARGET_ID", "1", "Server ID" } }, cTrl } );
            m_scripts.push_back( { "Troll", "TP to Player", "Teleport to target",
                "local target = GetPlayerPed(GetPlayerFromServerId(%TARGET_ID%))\nif target ~= 0 then\n  local c = GetEntityCoords(target)\n  SetEntityCoords(PlayerPedId(), c.x, c.y, c.z, false, false, false, false)\nend",
                { { "TARGET_ID", "1", "Server ID" } }, cTrl } );
            m_scripts.push_back( { "Troll", "Clone Player", "Spawn their model clone",
                "local target = GetPlayerPed(GetPlayerFromServerId(%TARGET_ID%))\nif target ~= 0 then\n  local hash = GetEntityModel(target)\n  RequestModel(hash)\n  while not HasModelLoaded(hash) do Citizen.Wait(10) end\n  local c = GetEntityCoords(target)\n  CreatePed(4, hash, c.x+1.0, c.y, c.z, GetEntityHeading(target), true, false)\nend",
                { { "TARGET_ID", "1", "Server ID" } }, cTrl } );
            m_scripts.push_back( { "Troll", "Force Ragdoll", "Make target ragdoll",
                "local target = GetPlayerPed(GetPlayerFromServerId(%TARGET_ID%))\nif target ~= 0 then\n  SetPedToRagdoll(target, 5000, 5000, 0, false, false, false)\nend",
                { { "TARGET_ID", "1", "Server ID" } }, cTrl } );
            m_scripts.push_back( { "Troll", "Kill Nearby Peds", "Kill all non-player peds",
                "local c = GetEntityCoords(PlayerPedId())\nfor _,ped in ipairs(GetGamePool('CPed')) do\n  if not IsPedAPlayer(ped) then\n    local pc = GetEntityCoords(ped)\n    if #(c - pc) < 100.0 then\n      SetEntityHealth(ped, 0)\n    end\n  end\nend", {}, cTrl } );
            m_scripts.push_back( { "Troll", "Explode All Vehicles", "Blow up nearby vehicles",
                "local c = GetEntityCoords(PlayerPedId())\nfor _,veh in ipairs(GetGamePool('CVehicle')) do\n  local vc = GetEntityCoords(veh)\n  if #(c - vc) < 100.0 then\n    NetworkExplodeVehicle(veh, true, true, false)\n  end\nend", {}, cTrl } );
            m_scripts.push_back( { "Troll", "Sound Spam", "Spam sounds at player",
                "local target = GetPlayerPed(GetPlayerFromServerId(%TARGET_ID%))\nif target ~= 0 then\n  local c = GetEntityCoords(target)\n  PlaySoundFromCoord(-1, 'Beep_Red', c.x, c.y, c.z, 'DLC_HEIST_HACKING_SNAKE_SOUNDS', true, 100, false)\nend",
                { { "TARGET_ID", "1", "Server ID" } }, cTrl } );
            m_scripts.push_back( { "Troll", "Fake Chat", "Send fake chat message",
                "TriggerServerEvent('chatMessage', '%NAME%', {0}, '%MESSAGE%')",
                { { "NAME", "Server", "Display name" }, { "MESSAGE", "Hello!", "Message" } }, cTrl } );

            // ── Utility ──
            m_scripts.push_back( { "Utility", "Dump Players", "List all connected players",
                "local out = ''\nfor _, pid in ipairs(GetActivePlayers()) do\n  local name = GetPlayerName(pid)\n  local sid = GetPlayerServerId(pid)\n  out = out .. sid .. ': ' .. name .. '\\n'\nend\nreturn out", {}, cUtl } );
            m_scripts.push_back( { "Utility", "Dump Resources", "List server resources",
                "local out = ''\nfor i=0, GetNumResources()-1 do\n  local name = GetResourceByFindIndex(i)\n  local state = GetResourceState(name)\n  out = out .. name .. ' [' .. state .. ']\\n'\nend\nreturn out", {}, cUtl } );
            m_scripts.push_back( { "Utility", "Server Info", "Get server information",
                "local host = GetConvar('sv_hostname', 'Unknown')\nlocal max = GetConvarInt('sv_maxclients', 32)\nlocal count = #GetActivePlayers()\nreturn 'Server: ' .. host .. '\\nPlayers: ' .. count .. '/' .. max", {}, cUtl } );
            m_scripts.push_back( { "Utility", "Copy Coords", "Get current position",
                "local c = GetEntityCoords(PlayerPedId())\nreturn string.format('%.4f, %.4f, %.4f', c.x, c.y, c.z)", {}, cUtl } );
            m_scripts.push_back( { "Utility", "Custom Lua", "Execute arbitrary code",
                "%CODE%", { { "CODE", "print('hello')", "Lua code" } }, cUtl } );
            m_scripts.push_back( { "Utility", "Console Command", "Run console cmd",
                "ExecuteCommand('%CMD%')", { { "CMD", "say hello", "Command" } }, cUtl } );
            m_scripts.push_back( { "Utility", "Local Player Info", "Detailed self info",
                "local p = PlayerPedId()\nlocal c = GetEntityCoords(p)\nlocal h = GetEntityHealth(p)\nlocal a = GetPedArmour(p)\nlocal w = GetSelectedPedWeapon(p)\nlocal v = GetVehiclePedIsIn(p, false)\nreturn string.format('Coords: %.2f, %.2f, %.2f\\nHealth: %d Armor: %d\\nWeapon: 0x%X\\nVehicle: %d', c.x, c.y, c.z, h, a, w, v)", {}, cUtl } );

            // ── Admin ──
            m_scripts.push_back( { "Admin", "Ban Player", "Ban via server event",
                "TriggerServerEvent('admin:banPlayer', %TARGET_ID%, '%REASON%')",
                { { "TARGET_ID", "1", "Server ID" }, { "REASON", "Cheating", "Reason" } }, cAdm } );
            m_scripts.push_back( { "Admin", "Kick Player", "Kick via server event",
                "TriggerServerEvent('admin:kickPlayer', %TARGET_ID%, '%REASON%')",
                { { "TARGET_ID", "1", "Server ID" }, { "REASON", "Kicked", "Reason" } }, cAdm } );
            m_scripts.push_back( { "Admin", "Start Resource", "Start a server resource",
                "TriggerServerEvent('admin:startResource', '%RESOURCE%')",
                { { "RESOURCE", "es_extended", "Resource name" } }, cAdm } );
            m_scripts.push_back( { "Admin", "Stop Resource", "Stop a server resource",
                "TriggerServerEvent('admin:stopResource', '%RESOURCE%')",
                { { "RESOURCE", "es_extended", "Resource name" } }, cAdm } );
            m_scripts.push_back( { "Admin", "Give Admin", "Grant self admin",
                "TriggerServerEvent('admin:givePermission', GetPlayerServerId(PlayerId()), 'admin')", {}, cAdm } );

            // ── Recovery ──
            m_scripts.push_back( { "Recovery", "QB Money Loop", "Repeated QB money give",
                "for i=1, %LOOPS% do\n  TriggerServerEvent('QBCore:Server:SetMoney', 'cash', %AMOUNT%)\n  Citizen.Wait(%INTERVAL%)\nend",
                { { "AMOUNT", "10000", "Per loop" }, { "LOOPS", "10", "Iterations" }, { "INTERVAL", "1000", "Delay ms" } }, cRec } );
            m_scripts.push_back( { "Recovery", "ESX Money Loop", "Repeated ESX money give",
                "for i=1, %LOOPS% do\n  TriggerServerEvent('esx:giveMoney', %AMOUNT%)\n  Citizen.Wait(%INTERVAL%)\nend",
                { { "AMOUNT", "10000", "Per loop" }, { "LOOPS", "10", "Iterations" }, { "INTERVAL", "1000", "Delay ms" } }, cRec } );
            m_scripts.push_back( { "Recovery", "Slot Machine", "Casino slot trigger",
                "TriggerServerEvent('casino:slots:win', %AMOUNT%)",
                { { "AMOUNT", "100000", "Win amount" } }, cRec } );
            m_scripts.push_back( { "Recovery", "Blackjack Win", "Casino blackjack trigger",
                "TriggerServerEvent('casino:blackjack:win', %AMOUNT%)",
                { { "AMOUNT", "50000", "Win amount" } }, cRec } );
            m_scripts.push_back( { "Recovery", "Dirty Money", "Give dirty money",
                "TriggerServerEvent('QBCore:Server:SetMoney', 'markedbills', %AMOUNT%)",
                { { "AMOUNT", "50000", "Amount" } }, cRec } );
        }

        std::vector<Script> m_scripts;
    };

    inline Library& g_Library = Library::Get();
}
