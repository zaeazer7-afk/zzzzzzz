#pragma once
#include <cstdint>

// FiveM native invoker — calls game natives via ScriptHookV-style interface
// Natives are hashed function calls into GTA V's engine

typedef void(*NativeHandler)(void* context);

class NativeContext {
    uint64_t args[32];
    uint32_t argCount;
    uint64_t result[4];
public:
    NativeContext() : argCount(0) {}

    template<typename T>
    void Push(T val) {
        static_assert(sizeof(T) <= 8);
        args[argCount++] = 0;
        *reinterpret_cast<T*>(&args[argCount - 1]) = val;
    }

    template<typename T>
    T GetResult() {
        return *reinterpret_cast<T*>(result);
    }

    void* GetArgsPtr() { return args; }
    uint32_t GetArgCount() { return argCount; }
    void* GetResultPtr() { return result; }
};

// native hashes — add more as needed from nativedb.dotindustries.dev
namespace NativeHash {
    // ENTITY
    constexpr uint64_t SET_ENTITY_INVINCIBLE      = 0x3882114BDE571AD4;
    constexpr uint64_t SET_ENTITY_VISIBLE          = 0xEA1C610A04DB6BBB;
    constexpr uint64_t SET_ENTITY_COORDS           = 0x06843DA7060A026B;
    constexpr uint64_t SET_ENTITY_COLLISION        = 0x1A9205C1B9EE827F;
    constexpr uint64_t GET_ENTITY_COORDS           = 0x3FEF770D40960D5A;

    // PLAYER
    constexpr uint64_t SET_PLAYER_INVINCIBLE       = 0x239528EACDC3E7DE;
    constexpr uint64_t CLEAR_PLAYER_WANTED_LEVEL   = 0xB302540597885499;
    constexpr uint64_t SET_MAX_WANTED_LEVEL        = 0xAA5F02DB8AEC006A;
    constexpr uint64_t SET_RUN_SPRINT_MULT         = 0x6DB47AA77FD94E09;
    constexpr uint64_t SET_SUPER_JUMP              = 0x57FFF03E423A4C0B;

    // PED
    constexpr uint64_t SET_PED_CAN_RAGDOLL         = 0xB128377056A54E2A;
    constexpr uint64_t GET_PED_MAX_HEALTH           = 0x4700A416E8324EF3;
    constexpr uint64_t SET_ENTITY_HEALTH            = 0x6B76DC1F3AE6E6A3;
    constexpr uint64_t SET_PED_ARMOUR               = 0xCEA04D83135571A6;

    // WEAPON
    constexpr uint64_t SET_PED_INFINITE_AMMO        = 0x3EDCB0505123623B;
    constexpr uint64_t GIVE_WEAPON_TO_PED           = 0xBF0FD6E56C964FCB;

    // VEHICLE
    constexpr uint64_t SET_VEHICLE_FIXED            = 0x115722B1B9C14C1C;
    constexpr uint64_t SET_VEHICLE_DEFORMATION_FIXED = 0x953DA1E1B12C0491;
    constexpr uint64_t CREATE_VEHICLE               = 0xAF35D0D2583051B0;
    constexpr uint64_t SET_VEHICLE_ON_GROUND        = 0x49733E92263139D1;

    // MISC
    constexpr uint64_t SET_WEATHER_TYPE_NOW          = 0x29B487C359E19889;
    constexpr uint64_t SET_CLOCK_TIME                = 0x47C3B5848C3E45D8;

    // UI
    constexpr uint64_t GET_FIRST_BLIP_INFO_ID       = 0x1BEDE233E6CD2A1F;
    constexpr uint64_t GET_BLIP_COORDS               = 0x586AFE3FF72D996E;

    // PLAYER ID
    constexpr uint64_t PLAYER_ID                     = 0x4F8644AF03DED0E0;
    constexpr uint64_t PLAYER_PED_ID                 = 0xD80958FC74E988A6;
    constexpr uint64_t GET_PLAYER_PED                = 0x43A66C31C68491C0;
}
