-- Turns damage events into blood: particles, splatter, screen blood and splashback.
local U = BFX.Util
local S = BFX.Settings
local Fx = BFX.Fx

local Hits = {
    lastHit = {},  -- ped -> time of the last blood-drawing hit (bleeding uses this)
    lastFx = {},   -- ped -> time effects last played (throttle)
    ignore = {},   -- peds handled elsewhere (the studio's test dummy)
    lastHealth = nil,
}
BFX.Hits = Hits

-- How each kind of damage behaves.
local KINDS = {
    bullet    = { scale = 1.0,  wall = true,  reach = 1.0, drops = 2, screen = 0.75 },
    shotgun   = { scale = 1.25, wall = true,  reach = 0.8, drops = 4, screen = 0.95 },
    sniper    = { scale = 1.4,  wall = true,  reach = 1.6, drops = 3, screen = 1.0, heavy = true },
    melee     = { scale = 0.7,  wall = false, reach = 0.4, drops = 1, screen = 0.55 },
    blade     = { scale = 0.9,  wall = true,  reach = 0.35, drops = 3, screen = 0.7 },
    explosion = { scale = 1.6,  wall = false, reach = 1.0, drops = 6, screen = 1.1, heavy = true, radial = true },
    vehicle   = { scale = 0.9,  wall = false, reach = 0.6, drops = 3, screen = 0.8 },
    gore      = { scale = 1.8,  wall = true,  reach = 1.2, drops = 6, screen = 1.2, heavy = true, radial = true },
}
Hits.kinds = KINDS

local function hashSet(names)
    local set = {}
    for _, name in ipairs(names) do set[U.u32(GetHashKey(name))] = true end
    return set
end

local NO_BLOOD = hashSet({
    'WEAPON_FALL', 'WEAPON_DROWNING', 'WEAPON_DROWNING_IN_VEHICLE', 'WEAPON_BLEEDING', 'WEAPON_EXHAUSTION',
    'WEAPON_ELECTRIC_FENCE', 'WEAPON_FIRE', 'WEAPON_MOLOTOV', 'WEAPON_STUNGUN', 'WEAPON_STUNGUN_MP',
    'WEAPON_BZGAS', 'WEAPON_SMOKEGRENADE', 'WEAPON_FIREEXTINGUISHER', 'WEAPON_PETROLCAN', 'WEAPON_HAZARDCAN',
    'WEAPON_FERTILIZERCAN', 'WEAPON_SNOWBALL', 'WEAPON_BALL', 'WEAPON_FLARE', 'WEAPON_FLAREGUN',
    'WEAPON_HIT_BY_WATER_CANNON',
})
local BLADES = hashSet({
    'WEAPON_KNIFE', 'WEAPON_DAGGER', 'WEAPON_MACHETE', 'WEAPON_SWITCHBLADE', 'WEAPON_HATCHET',
    'WEAPON_BATTLEAXE', 'WEAPON_STONE_HATCHET', 'WEAPON_BOTTLE', 'WEAPON_ANIMAL', 'WEAPON_COUGAR',
    'WEAPON_BARBED_WIRE',
})
local VEHICLES = hashSet({ 'WEAPON_RUN_OVER_BY_CAR', 'WEAPON_RAMMED_BY_CAR' })
local GORE = hashSet({ 'VEHICLE_WEAPON_ROTORS' })
local EXPLOSIVES = hashSet({
    'WEAPON_EXPLOSION', 'WEAPON_GRENADE', 'WEAPON_STICKYBOMB', 'WEAPON_PIPEBOMB', 'WEAPON_PROXMINE',
    'WEAPON_RPG', 'WEAPON_HOMINGLAUNCHER', 'WEAPON_GRENADELAUNCHER', 'WEAPON_COMPACTLAUNCHER', 'WEAPON_FIREWORK',
})
local GROUP_MELEE = U.u32(GetHashKey('GROUP_MELEE'))
local GROUP_UNARMED = U.u32(GetHashKey('GROUP_UNARMED'))
local GROUP_SHOTGUN = U.u32(GetHashKey('GROUP_SHOTGUN'))
local GROUP_SNIPER = U.u32(GetHashKey('GROUP_SNIPER'))
local GROUP_THROWN = U.u32(GetHashKey('GROUP_THROWN'))

-- SKEL_Head, IK_Head, FACIAL_facialRoot and SKEL_Neck_1: bloodfx.dat packs treat the neck like the head.
local HEAD_BONES = { [31086] = true, [12844] = true, [65068] = true, [39317] = true }
local BONE_HEAD = 31086
local BONE_SPINE3 = 24818
local GUN_KINDS = { bullet = true, shotgun = true, sniper = true }

-- Returns a damage kind, or nil for damage that should not draw blood.
function Hits.classify(weapon)
    local w = U.u32(weapon)
    if w == 0 then return 'bullet' end
    if NO_BLOOD[w] then return nil end
    if GORE[w] then return 'gore' end
    if BLADES[w] then return 'blade' end
    if VEHICLES[w] then return 'vehicle' end
    if EXPLOSIVES[w] then return 'explosion' end
    local group = U.u32(GetWeapontypeGroup(weapon))
    if group == GROUP_MELEE or group == GROUP_UNARMED then return 'melee' end
    if group == GROUP_SHOTGUN then return 'shotgun' end
    if group == GROUP_SNIPER then return 'sniper' end
    if group == GROUP_THROWN then return 'explosion' end
    return 'bullet'
end

-- Where the victim was hit, and whether it was the head.
local function hitPosition(victim, attackerIsMe, kind)
    local x, y, z
    local okBone, bone = GetPedLastDamageBone(victim)
    if not U.truthy(okBone) or bone == 0 then bone = nil end

    -- For our own shots the game knows exactly where the bullet landed.
    if attackerIsMe and GUN_KINDS[kind] then
        local okImpact, impact = GetPedLastWeaponImpactCoord(PlayerPedId())
        if U.truthy(okImpact) and impact then
            local vp = GetEntityCoords(victim)
            if U.dist3(impact.x, impact.y, impact.z, vp.x, vp.y, vp.z) < 2.2 then
                x, y, z = impact.x, impact.y, impact.z
            end
        end
    end

    if not x and bone then
        local c = GetPedBoneCoords(victim, bone, 0.0, 0.0, 0.0)
        x, y, z = c.x, c.y, c.z
    end
    if not x then
        local c = GetPedBoneCoords(victim, BONE_SPINE3, 0.0, 0.0, 0.0)
        x, y, z = c.x, c.y, c.z
    end

    local headshot = bone ~= nil and HEAD_BONES[bone] == true
    if not headshot and GUN_KINDS[kind] then
        local head = GetPedBoneCoords(victim, BONE_HEAD, 0.0, 0.0, 0.0)
        headshot = U.dist3(x, y, z, head.x, head.y, head.z) < 0.16
    end
    return x, y, z, headshot
end

-- Direction the damage travelled, from the attacker towards the hit point.
local function attackDirection(attacker, attackerIsMe, x, y, z)
    local ox, oy, oz
    if attackerIsMe then
        local cam = GetFinalRenderedCamCoord()
        ox, oy, oz = cam.x, cam.y, cam.z
    elseif attacker and attacker ~= 0 and DoesEntityExist(attacker) then
        local c = GetEntityCoords(attacker)
        ox, oy, oz = c.x, c.y, c.z + (IsEntityAPed(attacker) and 0.5 or 0.0)
    end
    if ox then
        local dx, dy, dz, len = U.normalize(x - ox, y - oy, z - oz)
        if len > 0.05 then return dx, dy, dz end
    end
    local a = math.random() * math.pi * 2.0
    local dx, dy, dz = U.normalize(math.cos(a), math.sin(a), -0.15)
    return dx, dy, dz
end

local lastScreen = 0
local function onLocalDamage(spec, headshot, fatal)
    if not S.get('enabled') or not S.get('screen.enabled') then return end
    local me = PlayerPedId()
    local hp = GetEntityHealth(me)
    local delta = (Hits.lastHealth or hp) - hp
    Hits.lastHealth = hp
    -- Armour soaked the hit: no wound, no blood.
    if delta <= 0 and not fatal and GetPedArmour(me) > 0 then return end
    -- One shotgun blast is several damage events; show it once.
    local now = GetGameTimer()
    if now - lastScreen < 100 and not fatal then return end
    lastScreen = now

    local intensity = spec.screen * S.get('screen.intensity')
    if delta > 0 then intensity = intensity * U.clamp(0.6 + delta / 60.0, 0.6, 1.4) end
    if headshot then intensity = intensity * 1.3 end
    if fatal then intensity = intensity * 1.4 end
    intensity = math.min(intensity, 2.0)

    SendNUIMessage({
        type = 'screen:hit',
        color = S.hex(),
        intensity = intensity,
        duration = S.get('screen.duration') * (fatal and 1.6 or 1.0),
        headshot = headshot,
    })

    local shake = S.get('screen.shake')
    if shake > 0 then
        ShakeGameplayCam('SMALL_EXPLOSION_SHAKE', shake * 0.3 * math.min(intensity, 1.2))
    end
end

local lastSplash = 0
local function splashback(me, vx, vy, vz, spec)
    if not S.get('hit.splashback') then return end
    local now = GetGameTimer()
    if now - lastSplash < 1200 then return end
    local mp = GetEntityCoords(me)
    local reach = Config.Splashback.distance * (spec.heavy and 1.5 or 1.0)
    if U.dist3(mp.x, mp.y, mp.z, vx, vy, vz) > reach then return end
    lastSplash = now
    local packs = Config.Splashback.packs
    ApplyPedDamagePack(me, packs[math.random(#packs)], 0.0, 1.0)
end

--[[
    Handles one damage event. `weapon` is a weapon hash (0 when unknown).
    Called for game damage events and directly by the studio's test dummy.
]]
function Hits.handle(victim, attacker, weapon, fatal)
    if not S.get('enabled') then return end
    if not victim or victim == 0 or not DoesEntityExist(victim) or not IsEntityAPed(victim) then return end

    local kind = Hits.classify(weapon or 0)
    if not kind then return end
    local spec = KINDS[kind]
    local now = GetGameTimer()
    local me = PlayerPedId()
    local attackerIsMe = attacker == me and victim ~= me
    fatal = fatal or IsPedDeadOrDying(victim, true)
    Hits.lastHit[victim] = now

    local isMe = victim == me
    local inScope = S.get('hit.scope') ~= 'mine' or attackerIsMe
    if not isMe and not inScope then return end

    local vp = GetEntityCoords(victim)
    local cam = GetFinalRenderedCamCoord()
    if U.dist3(cam.x, cam.y, cam.z, vp.x, vp.y, vp.z) > Config.Performance.effectRange then return end

    -- Shotguns and automatic fire produce bursts of events; play effects at most every 70 ms per ped.
    local last = Hits.lastFx[victim]
    local throttled = last ~= nil and now - last < 70 and not fatal

    local x, y, z, headshot = hitPosition(victim, attackerIsMe, kind)
    if isMe then onLocalDamage(spec, headshot, fatal) end
    if throttled or not inScope then return end
    Hits.lastFx[victim] = now

    local dx, dy, dz = attackDirection(attacker, attackerIsMe, x, y, z)
    Fx.hit(victim, x, y, z, dx, dy, dz, {
        kind = kind,
        spec = spec,
        headshot = headshot,
        fatal = fatal,
        inVehicle = IsPedInAnyVehicle(victim, false),
        inWater = IsEntityInWater(victim),
    })

    if attackerIsMe then splashback(me, vp.x, vp.y, vp.z, spec) end
end

-- The weapon hash moved one slot between game builds; take whichever slot holds a real weapon.
local function validWeapon(v)
    return math.type(v) == 'integer' and v ~= 0 and v ~= 1 and GetWeapontypeGroup(v) ~= 0
end

local function pickWeapon(args)
    local w = args[7]
    if validWeapon(w) then return w end
    if validWeapon(args[6]) then return args[6] end
    return math.type(w) == 'integer' and w or 0
end

AddEventHandler('gameEventTriggered', function(name, args)
    if name ~= 'CEventNetworkEntityDamage' then return end
    local victim = args[1]
    if not victim or Hits.ignore[victim] then return end
    Hits.handle(victim, args[2], pickWeapon(args), args[6] == 1)
end)

-- Low-health pulse, and the health baseline used to size screen blood.
CreateThread(function()
    local lastLevel = -1
    while true do
        local me = PlayerPedId()
        local hp = GetEntityHealth(me)
        if not Hits.lastHealth or hp > Hits.lastHealth then Hits.lastHealth = hp end

        local level = 0.0
        if S.get('enabled') and S.get('screen.enabled') and S.get('screen.lowHealth') and not IsEntityDead(me) then
            local ratio = U.healthRatio(hp, GetEntityMaxHealth(me))
            if ratio < 0.35 then level = U.round((0.35 - ratio) / 0.35, 2) end
        end
        if math.abs(level - lastLevel) >= 0.05 or (level == 0.0 and lastLevel ~= 0.0) then
            lastLevel = level
            SendNUIMessage({ type = 'screen:lowHealth', level = level, color = S.hex() })
        end
        Wait(400)
    end
end)

-- Forget peds that no longer exist.
CreateThread(function()
    while true do
        Wait(30000)
        local now = GetGameTimer()
        for _, map in ipairs({ Hits.lastHit, Hits.lastFx }) do
            for ped, t in pairs(map) do
                if now - t > 300000 or not DoesEntityExist(ped) then map[ped] = nil end
            end
        end
    end
end)
