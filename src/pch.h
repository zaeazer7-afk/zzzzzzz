#pragma once

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <d3d12.h>
#include <dxgi1_4.h>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <cstdarg>
#include <cmath>
#include <string>
#include <vector>
#include <array>
#include <unordered_map>
#include <atomic>
#include <mutex>

#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "dxgi.lib")

using u8  = uint8_t;
using u16 = uint16_t;
using u32 = uint32_t;
using u64 = uint64_t;
using i32 = int32_t;
using i64 = int64_t;
using f32 = float;

using Hash    = u32;
using Entity  = i32;
using Ped     = Entity;
using Vehicle = Entity;
using Player  = i32;
using Blip    = i32;
using Cam     = i32;
using BOOL    = int;

struct NativeVector3 {
    f32 x; u32 _pad0;
    f32 y; u32 _pad1;
    f32 z; u32 _pad2;
};

struct Vec3 {
    f32 x, y, z;
    Vec3() : x(0), y(0), z(0) {}
    Vec3(f32 x, f32 y, f32 z) : x(x), y(y), z(z) {}
};
