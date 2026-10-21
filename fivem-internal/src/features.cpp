// features.cpp — native execution loop (runs on game tick)
// FILL: hook into FiveM's script runtime to call natives each tick
// The tick function below is the handler — wire it into
// citizen-scripting's OnTick or a ScriptHookV-style tick callback.

#include "features.h"
#include "native.h"

// Placeholder: the actual native invocation mechanism depends on
// which hooking path you use (ScriptHookV / citizen-scripting / direct).
// This file shows the LOGIC; the invoker glue is target-specific.

/*
void Features::Tick() {
    int playerId = PLAYER_ID();
    int ped      = PLAYER_PED_ID();

    // -- self --
    if (Self::godMode)
        SET_PLAYER_INVINCIBLE(playerId, true);
    if (Self::neverWanted) {
        CLEAR_PLAYER_WANTED_LEVEL(playerId);
        SET_MAX_WANTED_LEVEL(0);
    }
    if (Self::noRagdoll)
        SET_PED_CAN_RAGDOLL(ped, false);
    if (Self::invisible)
        SET_ENTITY_VISIBLE(ped, false, false);
    if (Self::superJump)
        SET_SUPER_JUMP(playerId);
    if (Self::fastRun)
        SET_RUN_SPRINT_MULT(playerId, Self::speedMult);
    if (Self::maxHealth) {
        SET_ENTITY_HEALTH(ped, GET_PED_MAX_HEALTH(ped));
        Self::maxHealth = false;
    }
    if (Self::maxArmor) {
        SET_PED_ARMOUR(ped, 100);
        Self::maxArmor = false;
    }

    // -- weapon --
    if (Weapon::infAmmo)
        SET_PED_INFINITE_AMMO(ped, true, 0);

    // -- vehicle --
    int veh = GET_VEHICLE_PED_IS_IN(ped, false);
    if (veh && Vehicle::godMode)
        SET_ENTITY_INVINCIBLE(veh, true);
    if (veh && Vehicle::repair) {
        SET_VEHICLE_FIXED(veh);
        SET_VEHICLE_DEFORMATION_FIXED(veh);
        Vehicle::repair = false;
    }

    // -- teleport --
    if (Teleport::doTp) {
        SET_ENTITY_COORDS(ped, Teleport::tpX, Teleport::tpY, Teleport::tpZ, 0, 0, 0, 0);
        Teleport::doTp = false;
    }
    if (Teleport::toWaypoint) {
        int blip = GET_FIRST_BLIP_INFO_ID(8); // waypoint blip
        if (blip) {
            Vector3 c = GET_BLIP_COORDS(blip);
            SET_ENTITY_COORDS(ped, c.x, c.y, c.z, 0, 0, 0, 0);
        }
        Teleport::toWaypoint = false;
    }

    // -- misc --
    if (Misc::setWeather) {
        const char* weathers[] = {"CLEAR","EXTRASUNNY","CLOUDS","OVERCAST",
                                   "RAIN","CLEARING","THUNDER","SMOG",
                                   "FOGGY","XMAS","SNOWLIGHT","BLIZZARD"};
        SET_WEATHER_TYPE_NOW(weathers[Misc::weatherIdx]);
        Misc::setWeather = false;
    }
    if (Misc::setTime) {
        SET_CLOCK_TIME(Misc::hour, 0, 0);
        Misc::setTime = false;
    }
}
*/
