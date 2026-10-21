#include "vehicle.hpp"
#include "../natives/invoker.hpp"
#include "../natives/hashes.hpp"

namespace gta5::features {

using namespace gta5::natives;
using namespace gta5::natives::hash;

static Ped GetPlayerPed() {
    return NativeInvoker::Invoke<Ped>(PLAYER_PED_ID);
}

static Vehicle GetCurrentVehicle() {
    Ped ped = GetPlayerPed();
    if (!ped) return 0;
    BOOL inVeh = NativeInvoker::Invoke<BOOL>(IS_PED_IN_ANY_VEHICLE, ped, FALSE_);
    if (!inVeh) return 0;
    return NativeInvoker::Invoke<Vehicle>(GET_VEHICLE_PED_IS_IN, ped, FALSE_);
}

void SpawnVehicle(Hash model) {
    NativeInvoker::Invoke<void>(REQUEST_MODEL, model);

    int timeout = 100;
    while (!NativeInvoker::Invoke<BOOL>(HAS_MODEL_LOADED, model) && timeout-- > 0) {
        NativeInvoker::Invoke<void>(WAIT, static_cast<int>(0));
    }

    if (!NativeInvoker::Invoke<BOOL>(HAS_MODEL_LOADED, model))
        return;

    Ped ped = GetPlayerPed();
    Vector3 pos = NativeInvoker::Invoke<Vector3>(GET_ENTITY_COORDS, ped, TRUE_);

    Vehicle veh = NativeInvoker::Invoke<Vehicle>(
        CREATE_VEHICLE, model,
        pos.x + 3.0f, pos.y, pos.z,
        0.0f, TRUE_, FALSE_, FALSE_);

    if (veh) {
        NativeInvoker::Invoke<void>(SET_VEHICLE_ON_GROUND_PROPERLY, veh, 5.0f);
        NativeInvoker::Invoke<void>(SET_PED_INTO_VEHICLE, ped, veh, static_cast<int>(-1));
        NativeInvoker::Invoke<void>(SET_VEHICLE_ENGINE_ON, veh, TRUE_, TRUE_, FALSE_);
    }

    NativeInvoker::Invoke<void>(SET_MODEL_AS_NO_LONGER_NEEDED, model);
}

void RepairVehicle() {
    Vehicle veh = GetCurrentVehicle();
    if (!veh) return;
    NativeInvoker::Invoke<void>(SET_VEHICLE_FIXED, veh);
    NativeInvoker::Invoke<void>(SET_VEHICLE_ENGINE_ON, veh, TRUE_, TRUE_, FALSE_);
}

void GodModeVehicle(bool toggle) {
    Vehicle veh = GetCurrentVehicle();
    if (!veh) return;
    NativeInvoker::Invoke<void>(SET_ENTITY_INVINCIBLE, veh, toggle ? TRUE_ : FALSE_);
}

void BoostSpeed(float force) {
    Vehicle veh = GetCurrentVehicle();
    if (!veh) return;
    NativeInvoker::Invoke<void>(SET_VEHICLE_FORWARD_SPEED, veh, force);
}

} // namespace gta5::features
