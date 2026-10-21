#pragma once
#include "../natives/types.hpp"

namespace gta5::features {

void SpawnVehicle(gta5::natives::Hash model);
void RepairVehicle();
void GodModeVehicle(bool toggle);
void BoostSpeed(float force = 30.0f);

} // namespace gta5::features
