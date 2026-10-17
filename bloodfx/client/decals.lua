-- Creates blood decals and keeps the number alive under control.
local U = BFX.Util

local Decals = { list = {} }
BFX.Decals = Decals

local perf = Config.Performance
local bucket = U.newBucket(perf.maxDecalsPerSecond, perf.maxDecalsPerSecond)
local SURFACE_OFFSET = 0.04

--[[
    Creates a decal without tracking it and returns its handle (or nil).
    spec: type, x, y, z            point on the surface
          nx, ny, nz               surface normal, pointing out of the surface
          w, h                     size in metres
          angle                    rotation around the normal in radians (random when missing)
          ax, ay, az               or a direction to line the decal up with
          r, g, b, a               colour and opacity, 0..1
]]
function Decals.create(spec)
    local nx, ny, nz = spec.nx or 0.0, spec.ny or 0.0, spec.nz or 1.0
    local angle = spec.angle
    if not angle and spec.ax then
        angle = U.surfaceAngle(nx, ny, nz, spec.ax, spec.ay, spec.az)
    end
    angle = angle or math.random() * math.pi * 2.0
    local sx, sy, sz = U.surfaceVector(nx, ny, nz, angle)
    local w = math.max(0.02, spec.w or 0.5)
    local h = math.max(0.02, spec.h or w)

    local handle = AddDecal(spec.type or 1010,
        spec.x + nx * SURFACE_OFFSET, spec.y + ny * SURFACE_OFFSET, spec.z + nz * SURFACE_OFFSET,
        -nx, -ny, -nz, sx, sy, sz, w, h,
        spec.r or 0.37, spec.g or 0.0, spec.b or 0.0, spec.a or 1.0,
        -1.0, false, false, false)

    if not handle or handle == 0 then return nil end
    return handle
end

function Decals.destroy(handle)
    if handle and IsDecalAlive(handle) then
        RemoveDecal(handle)
    end
end

-- Hands a decal to the manager: it is removed after `lifeMs`, or earlier when the cap is hit.
function Decals.track(handle, lifeMs, x, y, z)
    local list = Decals.list
    list[#list + 1] = { h = handle, exp = lifeMs and (GetGameTimer() + lifeMs) or nil, x = x, y = y, z = z }
    while #list > perf.maxDecals do
        local oldest = table.remove(list, 1)
        Decals.destroy(oldest.h)
    end
end

-- Budgeted and tracked: the normal way effects create decals.
function Decals.place(spec)
    if not U.takeToken(bucket, GetGameTimer()) then return nil end
    local handle = Decals.create(spec)
    if handle then
        Decals.track(handle, spec.life, spec.x, spec.y, spec.z)
    end
    return handle
end

function Decals.clearNear(x, y, z, radius)
    local kept, removed = {}, 0
    for _, d in ipairs(Decals.list) do
        if U.dist3(x, y, z, d.x, d.y, d.z) <= radius then
            Decals.destroy(d.h)
            removed = removed + 1
        else
            kept[#kept + 1] = d
        end
    end
    Decals.list = kept
    return removed
end

function Decals.clearAll()
    for _, d in ipairs(Decals.list) do
        Decals.destroy(d.h)
    end
    Decals.list = {}
end

CreateThread(function()
    while true do
        Wait(1000)
        local now = GetGameTimer()
        local list, kept, expired = Decals.list, {}, false
        for i = 1, #list do
            local d = list[i]
            if d.exp and now >= d.exp then
                Decals.destroy(d.h)
                expired = true
            else
                kept[#kept + 1] = d
            end
        end
        if expired then Decals.list = kept end
    end
end)
