-- Death pools that spread under bodies, and drops behind wounded peds.
local U = BFX.Util
local S = BFX.Settings
local Fx = BFX.Fx
local Decals = BFX.Decals

local Peds = {
    deaths = {},   -- ped -> { at, done }
    pools = {},    -- spreading pools
    bleeders = {}, -- ped -> { last, x, y, since }
}
BFX.Peds = Peds

local perf = Config.Performance
local poolCfg = Config.Pools
local bleedCfg = Config.Bleeding

local BONE_SPINE3 = 24818
local BONE_PELVIS = 11816

local function isDown(ped)
    if IsPedDeadOrDying(ped, true) then return true end
    local hook = Config.IsPedDowned
    if not hook then return false end
    local ok, result = pcall(hook, ped)
    return ok and result == true
end

-- Starts a pool on the surface at x, y, z. `ped` is optional; the pool stops when that ped gets up.
function Peds.startPool(ped, x, y, z, nx, ny, nz, maxSize)
    local r, g, b = S.rgb()
    local pool = {
        ped = ped, x = x, y = y, z = z, nx = nx or 0.0, ny = ny or 0.0, nz = nz or 1.0,
        started = GetGameTimer(),
        grow = math.max(1.0, S.get('pools.growTime')) * 1000,
        maxSize = (maxSize or S.get('pools.size')) * U.randf(0.85, 1.15),
        size = 0.0,
        angle = math.random() * math.pi * 2.0,
        aspect = U.randf(0.75, 1.0),
        r = r, g = g, b = b, a = S.get('opacity'),
    }
    Peds.pools[#Peds.pools + 1] = pool
    return pool
end

local function updatePools(now)
    for i = #Peds.pools, 1, -1 do
        local p = Peds.pools[i]
        local t = U.clamp((now - p.started) / p.grow, 0.0, 1.0)
        local eased = 1.0 - (1.0 - t) * (1.0 - t)
        local size = U.lerp(poolCfg.startSize, p.maxSize, eased)
        local finished = t >= 1.0

        if p.ped then
            if not DoesEntityExist(p.ped) or not isDown(p.ped) then
                finished = true
            else
                local c = GetEntityCoords(p.ped)
                if U.dist2(c.x, c.y, p.x, p.y) > 1.6 then finished = true end -- the body was moved
            end
        end

        if size - p.size >= 0.05 or (finished and not p.handle) then
            local handle = Decals.create({
                type = poolCfg.decal, x = p.x, y = p.y, z = p.z, nx = p.nx, ny = p.ny, nz = p.nz,
                w = size, h = size * p.aspect, angle = p.angle, r = p.r, g = p.g, b = p.b, a = p.a,
            })
            if handle then
                if p.handle then Decals.destroy(p.handle) end
                p.handle, p.size = handle, size
            end
        end

        if finished then
            if p.handle then
                Decals.track(p.handle, S.get('splatter.lifetime') * 1500, p.x, p.y, p.z)
            end
            table.remove(Peds.pools, i)
        end
    end
end

local function considerPool(ped, now)
    local d = Peds.deaths[ped]
    if not d then
        Peds.deaths[ped] = { at = now }
        return
    end
    if d.done or now - d.at < poolCfg.startDelay then return end
    if #Peds.pools >= perf.maxPools then return end
    if GetEntitySpeed(ped) > 0.35 then return end -- still ragdolling
    d.done = true
    if IsEntityInWater(ped) or IsPedInAnyVehicle(ped, false) then return end

    local c = GetPedBoneCoords(ped, BONE_SPINE3, 0.0, 0.0, 0.0)
    local hit, gx, gy, gz, nx, ny, nz = Fx.raycast(c.x, c.y, c.z + 0.6, c.x, c.y, c.z - 2.5, ped)
    if not hit or nz < 0.55 then return end -- no floor, or too steep for a pool
    Peds.startPool(ped, gx, gy, gz, nx, ny, nz)
end

local function bleed(c, now, threshold)
    local ped = c.ped
    if IsPedInAnyVehicle(ped, false) or IsEntityInWater(ped) then return end

    local b = Peds.bleeders[ped]
    if not b then
        b = { last = 0, x = c.x, y = c.y, since = now }
        Peds.bleeders[ped] = b
    end
    local lastHit = BFX.Hits.lastHit[ped] or b.since
    if bleedCfg.stopAfter > 0 and now - lastHit > bleedCfg.stopAfter * 1000 then return end

    local ratio = U.healthRatio(GetEntityHealth(ped), GetEntityMaxHealth(ped))
    local severity = U.clamp(1.0 - ratio / threshold, 0.0, 1.0)
    local interval = U.lerp(bleedCfg.slowest, bleedCfg.fastest, severity)
    local elapsed = now - b.last
    if elapsed < interval then return end
    -- Standing still drips a third as often, so puddles do not pile up.
    if U.dist2(c.x, c.y, b.x, b.y) < 0.3 and elapsed < interval * 3 then return end
    b.last, b.x, b.y = now, c.x, c.y

    local pelvis = GetPedBoneCoords(ped, BONE_PELVIS, 0.0, 0.0, 0.0)
    local px, py = pelvis.x + U.randf(-0.2, 0.2), pelvis.y + U.randf(-0.2, 0.2)
    local hit, gx, gy, gz, nx, ny, nz = Fx.raycast(px, py, pelvis.z, px, py, pelvis.z - 3.0, ped)
    if not hit then return end

    local r, g, bl = S.rgb()
    local size = U.randf(0.06, 0.14) * (1.0 + severity) * S.get('splatter.size')
    Decals.place({
        type = math.random() < 0.65 and 1110 or 1010, x = gx, y = gy, z = gz, nx = nx, ny = ny, nz = nz,
        w = size, h = size, r = r, g = g, b = bl, a = S.get('opacity'), life = S.get('splatter.lifetime') * 1000,
    })
end

local function scan(now)
    local poolsOn, bleedOn = S.get('pools.enabled'), S.get('bleed.enabled')
    if not poolsOn and not bleedOn then return end

    local origin = GetEntityCoords(PlayerPedId())
    local range = perf.pedScanRange
    local poolNpcs, bleedNpcs = S.get('pools.npcs'), S.get('bleed.npcs')
    local threshold = S.get('bleed.threshold')
    local candidates = {}

    for _, ped in ipairs(GetGamePool('CPed')) do
        local p = GetEntityCoords(ped)
        local dist = U.dist3(p.x, p.y, p.z, origin.x, origin.y, origin.z)
        if dist <= range then
            local isPlayer = IsPedAPlayer(ped)
            if isDown(ped) then
                Peds.bleeders[ped] = nil
                if poolsOn and (isPlayer or poolNpcs) then considerPool(ped, now) end
            else
                Peds.deaths[ped] = nil
                local wounded = bleedOn and (isPlayer or bleedNpcs)
                    and U.healthRatio(GetEntityHealth(ped), GetEntityMaxHealth(ped)) < threshold
                if wounded then
                    candidates[#candidates + 1] = { ped = ped, dist = dist, x = p.x, y = p.y }
                else
                    Peds.bleeders[ped] = nil
                end
            end
        end
    end

    table.sort(candidates, function(a, b) return a.dist < b.dist end)
    for i = 1, math.min(#candidates, perf.maxBleeders) do
        bleed(candidates[i], now, threshold)
    end
end

-- Stops and removes spreading pools near a point (cleaning).
function Peds.clearNear(x, y, z, radius)
    for i = #Peds.pools, 1, -1 do
        local p = Peds.pools[i]
        if U.dist3(x, y, z, p.x, p.y, p.z) <= radius then
            if p.handle then Decals.destroy(p.handle) end
            table.remove(Peds.pools, i)
        end
    end
end

function Peds.clear()
    for _, p in ipairs(Peds.pools) do
        if p.handle then Decals.destroy(p.handle) end
    end
    Peds.pools, Peds.deaths, Peds.bleeders = {}, {}, {}
end

CreateThread(function()
    local nextScan, nextPrune = 0, 0
    while true do
        local now = GetGameTimer()
        if #Peds.pools > 0 then updatePools(now) end
        if S.get('enabled') and now >= nextScan then
            nextScan = now + 300
            scan(now)
        end
        if now >= nextPrune then
            nextPrune = now + 10000
            for _, map in ipairs({ Peds.deaths, Peds.bleeders }) do
                for ped in pairs(map) do
                    if not DoesEntityExist(ped) then map[ped] = nil end
                end
            end
        end
        Wait(#Peds.pools > 0 and 150 or 300)
    end
end)
