#include "teleport.hpp"
#include "../natives/invoker.hpp"
#include "../natives/hashes.hpp"
#include "../natives/types.hpp"

namespace gta5::features {

using namespace gta5::natives;
using namespace gta5::natives::hash;

static Ped GetPlayerPed() {
    return NativeInvoker::Invoke<Ped>(PLAYER_PED_ID);
}

void TeleportToCoords(float x, float y, float z) {
    Ped ped = GetPlayerPed();
    if (!ped) return;

    BOOL inVeh = NativeInvoker::Invoke<BOOL>(IS_PED_IN_ANY_VEHICLE, ped, FALSE_);
    Entity target = ped;
    if (inVeh) {
        target = NativeInvoker::Invoke<Vehicle>(GET_VEHICLE_PED_IS_IN, ped, FALSE_);
    }

    NativeInvoker::Invoke<void>(SET_ENTITY_COORDS, target, x, y, z, FALSE_, FALSE_, FALSE_, TRUE_);
}

void TeleportToWaypoint() {
    Blip waypointBlip = NativeInvoker::Invoke<Blip>(GET_FIRST_BLIP_INFO_ID, static_cast<int>(8));

    if (!NativeInvoker::Invoke<BOOL>(DOES_BLIP_EXIST, waypointBlip))
        return;

    Vector3 coords = NativeInvoker::Invoke<Vector3>(GET_BLIP_INFO_ID_COORD, waypointBlip);

    float groundZ = 0.0f;
    bool found = false;

    for (float testZ = 1000.0f; testZ >= 0.0f && !found; testZ -= 25.0f) {
        NativeInvoker::Invoke<void>(SET_ENTITY_COORDS,
            GetPlayerPed(), coords.x, coords.y, testZ,
            FALSE_, FALSE_, FALSE_, TRUE_);
        NativeInvoker::Invoke<void>(WAIT, static_cast<int>(50));

        BOOL result = NativeInvoker::Invoke<BOOL>(
            GET_GROUND_Z_FOR_3D_COORD,
            coords.x, coords.y, testZ,
            reinterpret_cast<uint64_t>(&groundZ),
            FALSE_, FALSE_);

        if (result && groundZ > 0.0f) {
            found = true;
        }
    }

    if (!found)
        groundZ = 300.0f;

    TeleportToCoords(coords.x, coords.y, groundZ + 1.0f);
}

} // namespace gta5::features
