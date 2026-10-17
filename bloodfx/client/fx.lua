-- Particle and splatter primitives used by hits, pools, the studio and exports.
local U = BFX.Util
local S = BFX.Settings
local Decals = BFX.Decals

local Fx = {}
BFX.Fx = Fx

local perf = Config.Performance
local particleBucket = U.newBucket(perf.maxParticlesPerSecond, perf.maxParticlesPerSecond)
local castRay = StartExpensiveSynchronousShapeTestLosProbe or StartShapeTestRay
local RAY_FLAGS = 1 | 16 -- map collision and objects; peds and vehicles are ignored

-- ------------------------------------------------------------------ raycasts

-- Returns hit, x, y, z, nx, ny, nz.
function Fx.raycast(x1, y1, z1, x2, y2, z2, ignore)
    local handle = castRay(x1, y1, z1, x2, y2, z2, RAY_FLAGS, ignore or 0, 7)
    local _, hit, pos, normal = GetShapeTestResult(handle)
    if not U.truthy(hit) or not pos then return false end
    return true, pos.x, pos.y, pos.z, normal.x, normal.y, normal.z
end

function Fx.ground(x, y, z, ignore, depth)
    return Fx.raycast(x, y, z + 0.5, x, y, z - (depth or 3.0), ignore)
end

-- ------------------------------------------------------------------ particles

local assets = {}  -- dictionary -> 'loading' | 'ready' | 'failed'
local stats = {}   -- effect name -> { ok, fail }

function Fx.requestAsset(asset)
    local state = assets[asset]
    if state == 'ready' then return true end
    if state == 'failed' then return false end
    if HasNamedPtfxAssetLoaded(asset) then
        assets[asset] = 'ready'
        return true
    end
    if state ~= 'loading' then
        assets[asset] = 'loading'
        RequestNamedPtfxAsset(asset)
        CreateThread(function()
            local deadline = GetGameTimer() + 10000
            while not HasNamedPtfxAssetLoaded(asset) and GetGameTimer() < deadline do
                Wait(50)
            end
            if HasNamedPtfxAssetLoaded(asset) then
                assets[asset] = 'ready'
            else
                assets[asset] = 'failed'
                U.debug('particle dictionary "%s" did not load', asset)
            end
        end)
    end
    return false
end

local function eachLayerList(fn)
    for _, style in ipairs(Config.HitStyles or {}) do
        for _, group in ipairs({ 'hit', 'headshot', 'kill', 'melee', 'blade' }) do
            if style[group] then fn(style[group]) end
        end
    end
    for _, list in pairs(Config.WeaponEffects or {}) do fn(list) end
end

function Fx.preload()
    eachLayerList(function(list)
        for _, layer in ipairs(list) do Fx.requestAsset(layer.asset or 'core') end
    end)
end

-- Starts the first effect name in the layer that works. Names that never start are skipped later.
function Fx.spawn(layer, x, y, z, rx, ry, rz, scale, tint, r, g, b)
    local asset = layer.asset or 'core'
    if not Fx.requestAsset(asset) then return false end
    local names = layer.names or { layer.name }
    for i = 1, #names do
        local name = names[i]
        local st = stats[name]
        if not st then
            st = { ok = 0, fail = 0 }
            stats[name] = st
        end
        if st.ok > 0 or st.fail < 3 then
            UseParticleFxAssetNextCall(asset)
            if tint then SetParticleFxNonLoopedColour(r, g, b) end
            if layer.alpha then SetParticleFxNonLoopedAlpha(layer.alpha) end
            local started = StartParticleFxNonLoopedAtCoord(name, x, y, z, rx, ry, rz, scale, false, false, false)
            if started == false or started == 0 then
                st.fail = st.fail + 1
                if st.fail == 3 and st.ok == 0 then
                    U.debug('particle "%s" in "%s" never started; using fallbacks', name, asset)
                end
            else
                st.ok = st.ok + 1
                return true
            end
        end
    end
    return false
end

local function layerRotation(layer, dx, dy, dz)
    local mode = layer.dir or 'forward'
    if mode == 'none' then return 0.0, 0.0, 0.0 end
    if mode == 'up' then return 0.0, 0.0, math.random() * 360.0 end
    if mode == 'back' then dx, dy, dz = -dx, -dy, -dz end
    if layer.axis == 'y' then return U.rotationY(dx, dy, dz) end
    return U.rotationZ(dx, dy, dz)
end

function Fx.layers(layers, x, y, z, dx, dy, dz, scale, tint, r, g, b)
    if not layers then return end
    local now = GetGameTimer()
    for i = 1, #layers do
        local layer = layers[i]
        if (not layer.chance or math.random() < layer.chance) and U.takeToken(particleBucket, now) then
            local off = layer.offset or 0.0
            local rx, ry, rz = layerRotation(layer, dx, dy, dz)
            Fx.spawn(layer, x + dx * off, y + dy * off, z + dz * off, rx, ry, rz,
                scale * (layer.scale or 1.0) * U.randf(0.9, 1.1), tint, r, g, b)
        end
    end
end

local styleMap = {}
for _, style in ipairs(Config.HitStyles or {}) do
    styleMap[style.key] = style
end

function Fx.style(key)
    return styleMap[key] or (Config.HitStyles and Config.HitStyles[1])
end

-- Which particle layers a damage kind uses with the given style.
local function kindLayers(style, kind)
    if not style or not style.hit or #style.hit == 0 then return nil end
    local shared = Config.WeaponEffects or {}
    if kind == 'melee' then return style.melee or shared.melee end
    if kind == 'blade' then return style.blade or shared.blade end
    if kind == 'vehicle' or kind == 'explosion' or kind == 'gore' then return shared[kind] end
    return style.hit
end

-- ------------------------------------------------------------------ splatter

local function decalSpec(ctx, decalType, x, y, z, nx, ny, nz, w, h)
    return {
        type = decalType, x = x, y = y, z = z, nx = nx, ny = ny, nz = nz, w = w, h = h,
        r = ctx.r, g = ctx.g, b = ctx.b, a = ctx.a, life = ctx.life,
    }
end

local function splatType()
    return math.random() < 0.5 and 1010 or 1110
end

-- Colour, opacity, size and lifetime from the current settings.
function Fx.decalContext()
    local r, g, b = S.rgb()
    return {
        r = r, g = g, b = b,
        a = S.get('opacity'),
        life = S.get('splatter.lifetime') * 1000,
        size = S.get('splatter.size'),
        reach = S.get('splatter.reach'),
    }
end

-- Blood carried along the bullet onto the first wall or floor behind the target.
function Fx.wallSpray(x, y, z, dx, dy, dz, ctx)
    local reach = ctx.reach
    local hit, hx, hy, hz, nx, ny, nz = Fx.raycast(
        x + dx * 0.25, y + dy * 0.25, z + dz * 0.25,
        x + dx * reach, y + dy * reach, z + dz * reach, ctx.ignore)
    if not hit then return false end

    local dist = U.dist3(x, y, z, hx, hy, hz)
    local base = 0.55 * ctx.size * U.lerp(1.15, 0.5, U.clamp(dist / reach, 0.0, 1.0))

    local streak = decalSpec(ctx, 1015, hx, hy, hz, nx, ny, nz, base * 1.5, base)
    streak.ax, streak.ay, streak.az = dx, dy, dz
    Decals.place(streak)
    Decals.place(decalSpec(ctx, splatType(), hx, hy, hz, nx, ny, nz, base * 0.7, base * 0.7))
    if ctx.heavy then
        local mist = decalSpec(ctx, 1017, hx, hy, hz, nx, ny, nz, base * 2.0, base * 2.0)
        mist.a = ctx.a * 0.8
        Decals.place(mist)
    end
    return true
end

-- Drops on the ground, flung forward along the hit direction.
function Fx.groundDrops(x, y, z, dx, dy, ctx, count)
    local len = math.sqrt(dx * dx + dy * dy)
    local hx, hy
    if len < 0.05 then
        local a = math.random() * math.pi * 2.0
        hx, hy = math.cos(a), math.sin(a)
    else
        hx, hy = dx / len, dy / len
    end
    local spread = math.max(0.4, ctx.reach * 0.3)
    for _ = 1, count do
        local d = U.randf(0.05, spread)
        local side = U.randf(-0.3, 0.3)
        local px, py = x + hx * d - hy * side, y + hy * d + hx * side
        local hit, gx, gy, gz, nx, ny, nz = Fx.raycast(px, py, z + 0.3, px, py, z - 3.5, ctx.ignore)
        if hit then
            local s = U.randf(0.08, 0.3) * ctx.size
            Decals.place(decalSpec(ctx, splatType(), gx, gy, gz, nx, ny, nz, s, s))
        end
    end
end

-- Blood thrown in every direction (explosions, rotor blades).
function Fx.radial(x, y, z, ctx, count)
    local spread = math.max(0.5, ctx.reach / 4.0)
    for _ = 1, count do
        local a = math.random() * math.pi * 2.0
        local d = U.randf(0.15, 1.6) * spread
        local px, py = x + math.cos(a) * d, y + math.sin(a) * d
        local hit, gx, gy, gz, nx, ny, nz = Fx.raycast(px, py, z + 0.3, px, py, z - 3.5, ctx.ignore)
        if hit then
            local s = U.randf(0.18, 0.55) * ctx.size
            Decals.place(decalSpec(ctx, math.random() < 0.3 and 1017 or 1010, gx, gy, gz, nx, ny, nz, s, s))
        end
    end
end

--[[
    Plays a full hit at a point.
    info: kind ('bullet', 'shotgun', 'sniper', 'melee', 'blade', 'explosion', 'vehicle', 'gore')
          spec (table from client/hits.lua: scale, wall, reach, drops, heavy, radial)
          headshot, fatal, inVehicle, inWater
]]
function Fx.hit(victim, x, y, z, dx, dy, dz, info)
    if not S.get('enabled') then return end
    local spec = info.spec
    local r, g, b = S.rgb()
    local boost = info.headshot and S.get('hit.headshot') or 1.0

    if S.get('hit.enabled') then
        local style = Fx.style(S.get('hit.style'))
        local layers = kindLayers(style, info.kind)
        if layers then
            local scale = S.get('hit.scale') * spec.scale * boost
            local tint = S.get('tint')
            Fx.layers(layers, x, y, z, dx, dy, dz, scale, tint, r, g, b)
            if info.headshot then Fx.layers(style.headshot, x, y, z, dx, dy, dz, scale, tint, r, g, b) end
            if info.fatal and S.get('hit.kill') then Fx.layers(style.kill, x, y, z, dx, dy, dz, scale, tint, r, g, b) end
        end
    end

    if not S.get('splatter.enabled') or info.inWater then return end
    local ctx = Fx.decalContext()
    ctx.r, ctx.g, ctx.b = r, g, b
    ctx.size = ctx.size * (info.headshot and (0.5 + boost * 0.5) or 1.0)
    ctx.reach = ctx.reach * spec.reach
    ctx.heavy = spec.heavy or info.headshot
    ctx.ignore = victim or 0

    local chance = math.min(1.0, S.get('splatter.chance') * (info.headshot and 1.6 or 1.0) * (info.fatal and 1.3 or 1.0))
    if S.get('splatter.wall') and spec.wall and not info.inVehicle and math.random() < chance then
        Fx.wallSpray(x, y, z, dx, dy, dz, ctx)
    end
    if S.get('splatter.ground') and not info.inVehicle then
        local count = spec.drops + (info.headshot and 2 or 0) + (info.fatal and 2 or 0)
        if spec.radial then
            Fx.radial(x, y, z, ctx, count)
        else
            Fx.groundDrops(x, y, z, dx, dy, ctx, count)
        end
    end
end

-- A single splatter on the ground under a point (exports, studio tests).
function Fx.splatterAt(x, y, z, size, r, g, b)
    local hit, gx, gy, gz, nx, ny, nz = Fx.ground(x, y, z, 0, 4.0)
    if not hit then return false end
    local ctx = Fx.decalContext()
    if r then ctx.r, ctx.g, ctx.b = r, g, b end
    local s = (size or 0.8) * ctx.size
    return Decals.place(decalSpec(ctx, 1010, gx, gy, gz, nx, ny, nz, s, s)) ~= nil
end
