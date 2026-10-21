#pragma once
#include "pch.h"

using NativeHandler = u64(*)(u64*);

struct NativeCallContext {
    u64* returnValue;
    u32  argCount;
    u64* args;
    u32  dataCount;
    alignas(16) u8 vectorSpace[192];
};

struct NativeRegistration {
    NativeRegistration* next;
    NativeHandler handlers[7];
    u32 count;
    u64 hashes[7];
};

namespace game {
    bool init();
    void shutdown();
    NativeHandler getHandler(u64 hash);

    uintptr_t localPed();
    uintptr_t localPlayerInfo();
    uintptr_t localVehicle();
    Vec3 localPos();

    template<typename Ret = void, typename... Args>
    inline Ret invoke(u64 hash, Args... args) {
        alignas(16) u64 argBuf[32] = {};
        int idx = 0;
        ((argBuf[idx++] = *(u64*)&args), ...);

        NativeCallContext ctx{};
        ctx.args        = argBuf;
        ctx.argCount    = sizeof...(Args);
        ctx.returnValue = argBuf;

        auto handler = getHandler(hash);
        if (handler) handler((u64*)&ctx);

        if constexpr (!std::is_void_v<Ret>) {
            return *(Ret*)argBuf;
        }
    }
}

namespace off {
    constexpr u64 Ped_Health       = 0x280;
    constexpr u64 Ped_MaxHealth    = 0x2A0;
    constexpr u64 Ped_Armor        = 0x14E0;
    constexpr u64 Ped_Navigation   = 0x30;
    constexpr u64 Ped_PlayerInfo   = 0x10C8;
    constexpr u64 Ped_Vehicle      = 0x1030;
    constexpr u64 Ped_Ragdoll      = 0x10B8;
    constexpr u64 Ped_Invisible    = 0x2C;

    constexpr u64 Nav_PosX         = 0x50;
    constexpr u64 Nav_PosY         = 0x54;
    constexpr u64 Nav_PosZ         = 0x58;

    constexpr u64 PI_WantedLevel   = 0x888;
    constexpr u64 PI_WantedMax     = 0x88C;
    constexpr u64 PI_Stamina       = 0xCF0;
    constexpr u64 PI_FrameFlags    = 0x209;
    constexpr u64 PI_SwimSpeed     = 0xCF4;
    constexpr u64 PI_RunSpeed      = 0xD10;

    constexpr u64 Veh_Health       = 0x8E0;
    constexpr u64 Veh_HealthMax    = 0x8E4;
    constexpr u64 Veh_Body         = 0x8E8;
    constexpr u64 Veh_Engine       = 0x8EC;
    constexpr u64 Veh_Petrol       = 0x8F0;
    constexpr u64 Veh_Gravity      = 0xC1C;
    constexpr u64 Veh_Handling     = 0x918;
}

namespace nh {
    constexpr u64 PLAYER_ID                     = 0x4F8644AF895B4175;
    constexpr u64 PLAYER_PED_ID                 = 0xD80958FC74E988A0;
    constexpr u64 SET_PLAYER_INVINCIBLE         = 0x239528EACDC3E7DE;
    constexpr u64 GET_PLAYER_WANTED_LEVEL       = 0xE28E54788CE8F12D;
    constexpr u64 SET_PLAYER_WANTED_LEVEL       = 0x39FF19C64EF7DA5B;
    constexpr u64 SET_MAX_WANTED_LEVEL          = 0xAA5F02DB48D704B9;
    constexpr u64 CLEAR_PLAYER_WANTED_LEVEL     = 0xB302540597885499;
    constexpr u64 SET_POLICE_IGNORE_PLAYER      = 0x32C6AA6B31F16284;
    constexpr u64 SET_EVERYONE_IGNORE_PLAYER    = 0x8D8ADB562F09A245;
    constexpr u64 SET_RUN_SPRINT_MULTIPLIER     = 0x6DB47AA77FD94E09;
    constexpr u64 SET_SWIM_MULTIPLIER           = 0xA91C6F0FF7D16A13;
    constexpr u64 SPECIAL_ABILITY_FILL_METER    = 0x3DACA8DDC6FD4980;
    constexpr u64 SET_SUPER_JUMP_THIS_FRAME     = 0x57FFF03E423A4C0B;
    constexpr u64 SET_EXPLOSIVE_AMMO_THIS_FRAME = 0xA0696A65F009EE18;
    constexpr u64 SET_FIRE_AMMO_THIS_FRAME      = 0x11879CDD803D30F4;
    constexpr u64 SET_EXPLOSIVE_MELEE_THIS_FRAME= 0xFF1BED81BFDC0FE0;

    constexpr u64 GET_ENTITY_COORDS             = 0x3FEF770D40960D5A;
    constexpr u64 SET_ENTITY_COORDS             = 0x06843DA7060A026B;
    constexpr u64 GET_ENTITY_HEADING            = 0xE83D4F9BA2A85C7A;
    constexpr u64 SET_ENTITY_HEADING            = 0x8E2530AA8ADA980E;
    constexpr u64 SET_ENTITY_INVINCIBLE         = 0x3882114BDE571AD4;
    constexpr u64 SET_ENTITY_VISIBLE            = 0xEA1C610A04DB6BBB;
    constexpr u64 GET_ENTITY_HEALTH             = 0xEEF059FAD016D209;
    constexpr u64 SET_ENTITY_HEALTH             = 0x6B76DC1F3AE6E6A3;
    constexpr u64 GET_ENTITY_MAX_HEALTH         = 0x15D757606D170C3C;
    constexpr u64 SET_ENTITY_MAX_HEALTH         = 0x166E7CF68597D8B5;
    constexpr u64 IS_ENTITY_DEAD                = 0x5F9532F3B5CC2551;
    constexpr u64 GET_ENTITY_SPEED              = 0xD5037BA82E12416F;
    constexpr u64 SET_ENTITY_VELOCITY           = 0x1C99BB7B6E96D16F;
    constexpr u64 FREEZE_ENTITY_POSITION        = 0x428CA6DBD1094446;
    constexpr u64 SET_ENTITY_COLLISION          = 0x1A9205C1B2BA1191;
    constexpr u64 SET_ENTITY_NO_LONGER_NEEDED   = 0xB736A491E64A32CF;

    constexpr u64 SET_PED_CAN_RAGDOLL           = 0xB128377056A54E2A;
    constexpr u64 SET_PED_ARMOUR                = 0xCEA04D83135264CC;
    constexpr u64 GET_PED_ARMOUR                = 0x9483AF821C54B1DA;
    constexpr u64 IS_PED_IN_ANY_VEHICLE         = 0x997ABD671D25CA0B;
    constexpr u64 GET_VEHICLE_PED_IS_IN         = 0x9C18F74878FC4CE0;
    constexpr u64 SET_PED_INTO_VEHICLE          = 0xF75B0D629E1C063D;
    constexpr u64 GIVE_WEAPON_TO_PED            = 0xBF0FD6E56C964FCB;
    constexpr u64 REMOVE_ALL_PED_WEAPONS        = 0xF25DF915FA38C5F3;
    constexpr u64 SET_PED_INFINITE_AMMO_CLIP    = 0x183DADC6AA953186;
    constexpr u64 SET_PED_INFINITE_AMMO         = 0x3EDCB0505123623B;
    constexpr u64 SET_PED_CONFIG_FLAG           = 0x1913FE4CBF41C463;

    constexpr u64 CREATE_VEHICLE                = 0xAF35D0D2583051B0;
    constexpr u64 SET_VEHICLE_FIXED             = 0x115722B1B9C14C1C;
    constexpr u64 SET_VEHICLE_ENGINE_ON         = 0x2497C4717C8B881E;
    constexpr u64 SET_VEHICLE_FORWARD_SPEED     = 0xAB54A438726D25D5;

    constexpr u64 REQUEST_MODEL                 = 0xC0296A2EDF545F92;
    constexpr u64 HAS_MODEL_LOADED              = 0x98A4EB5D89A0C952;
    constexpr u64 SET_MODEL_AS_NO_LONGER_NEEDED = 0xE532F5D78798DAAB;

    constexpr u64 GET_HASH_KEY                  = 0xD24D37CC275948CC;
    constexpr u64 GET_GROUND_Z_FOR_3D_COORD    = 0xC906A7DAB05C8D2B;
    constexpr u64 SET_WEATHER_TYPE_NOW_PERSIST  = 0xED712CA327900C8A;
    constexpr u64 CLEAR_WEATHER_TYPE_PERSIST    = 0xCCC39339BEF76CF5;

    constexpr u64 IS_WAYPOINT_ACTIVE            = 0x1DD1F58F493F1DA5;
    constexpr u64 GET_FIRST_BLIP_INFO_ID        = 0x1BEDE233E6CD2A1F;
    constexpr u64 GET_BLIP_INFO_ID_COORD        = 0xFA7C7F0AADF25D09;

    constexpr u64 SET_CLOCK_TIME                = 0x47C3B5848C3E45D8;
    constexpr u64 PAUSE_CLOCK                   = 0x4055E40BD2DBEC1D;
    constexpr u64 NETWORK_OVERRIDE_CLOCK_TIME   = 0xE679E3E06E363892;

    constexpr u64 STAT_SET_INT                  = 0xB3271D7AB655B441;
    constexpr u64 STAT_SET_FLOAT                = 0x4851997F37FE9B3C;
    constexpr u64 STAT_SET_BOOL                 = 0x4B33C4243DE0C432;
    constexpr u64 STAT_GET_INT                  = 0x767FBC2AC802EF3D;

    constexpr u64 SET_NIGHTVISION               = 0x18F621F7A5B1F85D;
    constexpr u64 SET_SEETHROUGH                = 0x44B80ABAB9D80BD3;
}

namespace sigs {
    constexpr auto WORLD      = "48 8B 05 ? ? ? ? 48 8B 48 08 48 85 C9 74 52 8B 81";
    constexpr auto NATIVE_REG = "48 83 EC 20 48 8D 0D ? ? ? ? E8 ? ? ? ? 48 8D 0D";
    constexpr auto SWAPCHAIN  = "48 8B 0D ? ? ? ? 48 8B 01 44 8D 43 01 33 D2 FF 50 40 8B C8";

    constexpr auto BE_DATA              = "48 C7 05 ? ? ? ? 00 00 00 00 E8 ? ? ? ? 48 89 C1 E8 ? ? ? ? E8 ? ? ? ? BD 0A 00 00 00";
    constexpr auto BE_STATUS_UPDATE     = "C6 05 ? ? ? ? 00 84 C0 0F 84 ? ? ? ? E9";
    constexpr auto BE_PROCESS_JOIN      = "48 89 10 48 89 48 10 89 F9";
}
