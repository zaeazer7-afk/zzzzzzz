#pragma once
#include <cstdint>

namespace gta5::natives {

// RAGE Vector3 — each float padded to 8 bytes in native returns
struct Vector3 {
    float x;
    uint32_t _padX;
    float y;
    uint32_t _padY;
    float z;
    uint32_t _padZ;

    Vector3() : x(0), _padX(0), y(0), _padY(0), z(0), _padZ(0) {}
    Vector3(float x, float y, float z) : x(x), _padX(0), y(y), _padY(0), z(z), _padZ(0) {}
};
static_assert(sizeof(Vector3) == 0x18);

using Hash    = uint32_t;
using Entity  = int32_t;
using Ped     = int32_t;
using Vehicle = int32_t;
using Object  = int32_t;
using Player  = int32_t;
using Cam     = int32_t;
using Blip    = int32_t;
using Pickup  = int32_t;
using Any     = int64_t;

// RAGE uses int for booleans in natives
using BOOL = int32_t;
constexpr BOOL TRUE_  = 1;
constexpr BOOL FALSE_ = 0;

} // namespace gta5::natives
