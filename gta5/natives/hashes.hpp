#pragma once
#include <cstdint>

namespace gta5::natives::hash {

// ENTITY
constexpr uint64_t SET_ENTITY_INVINCIBLE       = 0x3882114BDE571AD4;
constexpr uint64_t SET_ENTITY_HEALTH           = 0x6B76DC1F3AE6E6A3;
constexpr uint64_t GET_ENTITY_HEALTH           = 0xEEF059FAD016D209;
constexpr uint64_t GET_ENTITY_MAX_HEALTH       = 0x15D757606D170C3C;
constexpr uint64_t SET_ENTITY_MAX_HEALTH       = 0x166E7CF68597D8B5;
constexpr uint64_t SET_ENTITY_VISIBLE          = 0xEA1C610A04DB6BBB;
constexpr uint64_t SET_ENTITY_COORDS           = 0x06843DA7060A026B;
constexpr uint64_t GET_ENTITY_COORDS           = 0x3FEF770D40960D5A;
constexpr uint64_t SET_ENTITY_VELOCITY         = 0x1C99BB7B6E96D16F;
constexpr uint64_t GET_ENTITY_VELOCITY         = 0x4805D2B1D8CF94A9;
constexpr uint64_t GET_ENTITY_FORWARD_VECTOR   = 0x0A794A5A57F8DF91;
constexpr uint64_t GET_ENTITY_MODEL            = 0x9F47B058362C84B5;

// PLAYER
constexpr uint64_t PLAYER_PED_ID               = 0xD80958FC74E988A6;
constexpr uint64_t PLAYER_ID                   = 0x4F8644AF03DDB6BA;
constexpr uint64_t SET_PLAYER_WANTED_LEVEL     = 0x39FF19C64EF7DA5B;
constexpr uint64_t CLEAR_PLAYER_WANTED_LEVEL   = 0xB302540597885499;
constexpr uint64_t SET_PLAYER_WANTED_LEVEL_NOW = 0xE0A7D1E497FFCD6F;
constexpr uint64_t GET_PLAYER_WANTED_LEVEL     = 0xE28E54788CE8F12D;
constexpr uint64_t SET_MAX_WANTED_LEVEL        = 0xAA5F02DB8FB2FFC4;

// PED
constexpr uint64_t SET_PED_ARMOUR              = 0xCEA04D83135264CC;
constexpr uint64_t GET_PED_ARMOUR              = 0x9483AF821C54B65C;
constexpr uint64_t SET_PED_CAN_RAGDOLL         = 0xB128377056A54E2A;
constexpr uint64_t IS_PED_IN_ANY_VEHICLE       = 0x997ABD671D25CA0B;
constexpr uint64_t GET_VEHICLE_PED_IS_IN       = 0x9A9112A0FE9A4713;
constexpr uint64_t SET_PED_INTO_VEHICLE        = 0xF75B0D629E1C063D;
constexpr uint64_t IS_PED_JUMPING              = 0xCEDABC5900A0BF97;

// VEHICLE
constexpr uint64_t CREATE_VEHICLE              = 0xAF35D0D2583051B0;
constexpr uint64_t SET_VEHICLE_FIXED           = 0x115722B1B9C14C1C;
constexpr uint64_t SET_VEHICLE_ENGINE_ON       = 0x2497C4717C8B881E;
constexpr uint64_t SET_VEHICLE_ON_GROUND_PROPERLY = 0x49733E92263139D1;
constexpr uint64_t SET_VEHICLE_FORWARD_SPEED   = 0xAB54A438726D25D5;
constexpr uint64_t DELETE_VEHICLE              = 0xEA386986E786A54F;

// WEAPON
constexpr uint64_t GIVE_WEAPON_TO_PED          = 0xBF0FD6E56C964FCB;
constexpr uint64_t REMOVE_ALL_PED_WEAPONS      = 0xF25DF915FA38C5F3;

// STREAMING
constexpr uint64_t REQUEST_MODEL               = 0x963D27A58DF860AC;
constexpr uint64_t HAS_MODEL_LOADED            = 0x98A4EB5D89A0C952;
constexpr uint64_t SET_MODEL_AS_NO_LONGER_NEEDED = 0xE532F5D78798DAAB;

// HUD / BLIP
constexpr uint64_t GET_FIRST_BLIP_INFO_ID      = 0x1BEDE233E6CD2A1F;
constexpr uint64_t GET_BLIP_INFO_ID_COORD      = 0xFA7C7F0AADF25D09;
constexpr uint64_t DOES_BLIP_EXIST             = 0xA6DB27D19ECBB7DA;
constexpr uint64_t GET_NEXT_BLIP_INFO_ID       = 0x14F96AA50D6FBEA7;

// MISC
constexpr uint64_t GET_GROUND_Z_FOR_3D_COORD   = 0xC906A7DAB05C8D2B;
constexpr uint64_t GET_HASH_KEY                 = 0xD24D37379F2B3750;
constexpr uint64_t WAIT                         = 0x4EDE34FBADD967A6;
constexpr uint64_t IS_CONTROL_JUST_PRESSED      = 0x580417101DDB492F;

// GAMEPLAY
constexpr uint64_t GET_GAME_TIMER               = 0x9CD27B0045628463;

// Weapon model hashes
constexpr uint32_t WEAPON_PISTOL        = 0x1B06D571;
constexpr uint32_t WEAPON_COMBATPISTOL  = 0x5EF9FEC4;
constexpr uint32_t WEAPON_HEAVYPISTOL   = 0xD205520E;
constexpr uint32_t WEAPON_SMG           = 0x2BE6766B;
constexpr uint32_t WEAPON_ASSAULTRIFLE  = 0xBFEFFF6D;
constexpr uint32_t WEAPON_CARBINERIFLE  = 0x83BF0278;
constexpr uint32_t WEAPON_SPECIALCARBIN = 0xC0A3098D;
constexpr uint32_t WEAPON_SNIPERRIFLE   = 0x05FC3C11;
constexpr uint32_t WEAPON_HEAVYSNIPER   = 0x0C472FE2;
constexpr uint32_t WEAPON_RPG           = 0xB1CA77B1;
constexpr uint32_t WEAPON_MINIGUN       = 0x42BF8A85;
constexpr uint32_t WEAPON_PUMPSHOTGUN   = 0x1D073A89;
constexpr uint32_t WEAPON_COMBATSHOTGUN = 0x5A96BA4;
constexpr uint32_t WEAPON_MG            = 0x9D07F764;
constexpr uint32_t WEAPON_COMBATMG      = 0x7FD62962;
constexpr uint32_t WEAPON_GRENADELAUNCHER = 0xA284510B;
constexpr uint32_t WEAPON_STICKYBOMB    = 0x2C3731D9;
constexpr uint32_t WEAPON_GRENADE       = 0x93E220BD;
constexpr uint32_t WEAPON_MOLOTOV       = 0x24B17070;
constexpr uint32_t WEAPON_KNIFE         = 0x99B507EA;
constexpr uint32_t WEAPON_BAT           = 0x958A4A8F;
constexpr uint32_t WEAPON_MICROSMG      = 0x13532244;
constexpr uint32_t WEAPON_ADVANCEDRIFLE = 0xAF113F99;
constexpr uint32_t WEAPON_BULLPUPRIFLE  = 0x7F229F94;
constexpr uint32_t WEAPON_MARKSMANRIFLE = 0xC734385A;

} // namespace gta5::natives::hash
