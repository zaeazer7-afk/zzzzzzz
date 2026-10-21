#include "weapon.hpp"
#include "../natives/invoker.hpp"
#include "../natives/hashes.hpp"
#include "../natives/types.hpp"

namespace gta5::features {

using namespace gta5::natives;
using namespace gta5::natives::hash;

static constexpr uint32_t ALL_WEAPONS[] = {
    WEAPON_PISTOL,         WEAPON_COMBATPISTOL,  WEAPON_HEAVYPISTOL,
    WEAPON_SMG,            WEAPON_MICROSMG,
    WEAPON_ASSAULTRIFLE,   WEAPON_CARBINERIFLE,  WEAPON_SPECIALCARBIN,
    WEAPON_ADVANCEDRIFLE,  WEAPON_BULLPUPRIFLE,
    WEAPON_MG,             WEAPON_COMBATMG,
    WEAPON_PUMPSHOTGUN,    WEAPON_COMBATSHOTGUN,
    WEAPON_SNIPERRIFLE,    WEAPON_HEAVYSNIPER,   WEAPON_MARKSMANRIFLE,
    WEAPON_RPG,            WEAPON_GRENADELAUNCHER,WEAPON_MINIGUN,
    WEAPON_GRENADE,        WEAPON_STICKYBOMB,    WEAPON_MOLOTOV,
    WEAPON_KNIFE,          WEAPON_BAT,
};

void GiveAllWeapons() {
    Ped ped = NativeInvoker::Invoke<Ped>(PLAYER_PED_ID);
    if (!ped) return;

    for (uint32_t hash : ALL_WEAPONS) {
        NativeInvoker::Invoke<void>(GIVE_WEAPON_TO_PED, ped, hash, static_cast<int>(9999), FALSE_, TRUE_);
    }
}

void RemoveAllWeapons() {
    Ped ped = NativeInvoker::Invoke<Ped>(PLAYER_PED_ID);
    if (!ped) return;
    NativeInvoker::Invoke<void>(REMOVE_ALL_PED_WEAPONS, ped, TRUE_);
}

} // namespace gta5::features
