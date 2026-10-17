-- BloodFX Studio: the NUI editor, its test buttons and the test dummy.
local U = BFX.Util
local S = BFX.Settings
local Schema = BFX.Schema
local Fx = BFX.Fx
local Hits = BFX.Hits
local Peds = BFX.Peds
local Decals = BFX.Decals

local Studio = { state = 'closed', canPaint = false }
BFX.Studio = Studio

local Dummy = { ped = nil }
Studio.dummy = Dummy

local BONE_HEAD = 31086
local BONE_SPINE3 = 24818

local function keyHint()
    if Config.OpenKey and Config.OpenKey ~= '' then return Config.OpenKey end
    return '/' .. (Config.Commands.studio or 'bloodfx')
end

-- ------------------------------------------------------------------ open / close

function Studio.open()
    if Studio.state == 'detached' then
        Studio.focus()
        return
    end
    Studio.state = 'focused'
    SendNUIMessage({
        type = 'studio:open',
        settings = S.current,
        schema = Schema.export(),
        sections = Schema.sections,
        presets = Schema.presetCards(),
        canPaint = Studio.canPaint,
        dummy = Dummy.exists(),
        keyHint = keyHint(),
        customizable = Config.AllowPlayerCustomization ~= false,
        version = GetResourceMetadata(GetCurrentResourceName(), 'version', 0) or '',
    })
    SetNuiFocus(true, true)
    TriggerServerEvent('bloodfx:sv:painterAccess', true) -- refreshes the painter button
end

function Studio.focus()
    Studio.state = 'focused'
    SendNUIMessage({ type = 'studio:focus' })
    SetNuiFocus(true, true)
end

-- Keeps the panel on screen without the cursor, so you can play and watch the effects.
function Studio.detach()
    Studio.state = 'detached'
    SetNuiFocus(false, false)
end

function Studio.close()
    Studio.state = 'closed'
    SetNuiFocus(false, false)
    SendNUIMessage({ type = 'studio:close' })
end

function Studio.toggle()
    if Studio.state == 'focused' then
        Studio.close()
    else
        Studio.open()
    end
end

function Studio.setCanPaint(allowed)
    Studio.canPaint = allowed
    SendNUIMessage({ type = 'studio:canPaint', value = allowed })
end

-- ------------------------------------------------------------------ test dummy

function Dummy.exists()
    return Dummy.ped ~= nil and DoesEntityExist(Dummy.ped)
end

function Dummy.remove()
    local ped = Dummy.ped
    Dummy.ped = nil
    if ped then
        Hits.ignore[ped] = nil
        if DoesEntityExist(ped) then DeleteEntity(ped) end
    end
end

-- Damage to a local ped is watched directly, so effects match what you shot.
local function watch(ped)
    CreateThread(function()
        local last = GetEntityHealth(ped)
        while Dummy.ped == ped and DoesEntityExist(ped) do
            local hp = GetEntityHealth(ped)
            if hp < last then
                local me = PlayerPedId()
                Hits.handle(ped, me, GetSelectedPedWeapon(me), IsPedDeadOrDying(ped, true))
            end
            last = hp
            Wait(0)
        end
    end)
end

function Dummy.spawn()
    Dummy.remove()
    local model = GetHashKey(Config.Dummy.model)
    if not IsModelInCdimage(model) then
        BFX.notify(Config.Text.dummyFailed)
        return false
    end
    RequestModel(model)
    local deadline = GetGameTimer() + 5000
    while not HasModelLoaded(model) and GetGameTimer() < deadline do Wait(25) end
    if not HasModelLoaded(model) then
        BFX.notify(Config.Text.dummyFailed)
        return false
    end

    local me = PlayerPedId()
    local pos = GetOffsetFromEntityInWorldCoords(me, 0.0, 4.0, 0.0)
    local ped = CreatePed(4, model, pos.x, pos.y, pos.z - 1.0, GetEntityHeading(me) + 180.0, false, false)
    SetModelAsNoLongerNeeded(model)
    if not ped or ped == 0 then
        BFX.notify(Config.Text.dummyFailed)
        return false
    end

    SetEntityAsMissionEntity(ped, true, true)
    SetBlockingOfNonTemporaryEvents(ped, true)
    SetPedFleeAttributes(ped, 0, false)
    SetPedMaxHealth(ped, Config.Dummy.health)
    SetEntityHealth(ped, Config.Dummy.health)
    SetPedSuffersCriticalHits(ped, false)
    TaskStandStill(ped, -1)

    Dummy.ped = ped
    Hits.ignore[ped] = true
    watch(ped)
    BFX.notify(Config.Text.dummySpawned)
    return true
end

-- ------------------------------------------------------------------ tests

local function testTarget()
    if Dummy.exists() then return Dummy.ped end
    return PlayerPedId()
end

local function fakeHit(target, headshot, fatal)
    local c = GetPedBoneCoords(target, headshot and BONE_HEAD or BONE_SPINE3, 0.0, 0.0, 0.0)
    local cam = GetFinalRenderedCamCoord()
    local dx, dy, dz, len = U.normalize(c.x - cam.x, c.y - cam.y, c.z - cam.z)
    if len < 0.3 then -- first person on yourself: spray forward instead
        local f = GetEntityForwardVector(target)
        dx, dy, dz = f.x, f.y, -0.1
    end
    Fx.hit(target, c.x, c.y, c.z, dx, dy, dz, {
        kind = 'bullet', spec = Hits.kinds.bullet, headshot = headshot, fatal = fatal,
        inVehicle = IsPedInAnyVehicle(target, false), inWater = IsEntityInWater(target),
    })
end

local function testPool()
    local me = PlayerPedId()
    local p = GetOffsetFromEntityInWorldCoords(me, 0.0, 1.4, 0.0)
    local hit, gx, gy, gz, nx, ny, nz = Fx.ground(p.x, p.y, p.z, me, 3.0)
    if hit then Peds.startPool(nil, gx, gy, gz, nx, ny, nz) end
end

local function testScreen()
    SendNUIMessage({
        type = 'screen:hit', color = S.hex(), intensity = S.get('screen.intensity'),
        duration = S.get('screen.duration'), headshot = false,
    })
end

local function cleanUp()
    local me = PlayerPedId()
    local p = GetEntityCoords(me)
    Decals.clearNear(p.x, p.y, p.z, 40.0)
    Peds.clearNear(p.x, p.y, p.z, 40.0)
    ClearPedBloodDamage(me)
    ResetPedVisibleDamage(me)
    if Dummy.exists() then
        ClearPedBloodDamage(Dummy.ped)
        ResetPedVisibleDamage(Dummy.ped)
    end
    SendNUIMessage({ type = 'screen:clear' })
end

local TESTS = {
    hit = function() fakeHit(testTarget(), math.random() < 0.35, false) end,
    kill = function() fakeHit(testTarget(), true, true) end,
    pool = testPool,
    screen = testScreen,
    clean = cleanUp,
    dummy = function()
        if Dummy.exists() then Dummy.remove() else Dummy.spawn() end
    end,
}

function Studio.test(kind)
    local fn = TESTS[kind]
    if fn then fn() end
end

-- ------------------------------------------------------------------ NUI callbacks

local function tableArg(data, key)
    if type(data) ~= 'table' then return nil end
    return data[key]
end

RegisterNUICallback('close', function(_, cb)
    Studio.close()
    cb({ ok = true })
end)

RegisterNUICallback('detach', function(_, cb)
    Studio.detach()
    cb({ ok = true })
end)

RegisterNUICallback('update', function(data, cb)
    local values = tableArg(data, 'settings')
    cb({ settings = S.apply(type(values) == 'table' and values or nil) })
end)

RegisterNUICallback('preset', function(data, cb)
    local result = S.applyPreset(tableArg(data, 'key'))
    cb({ ok = result ~= nil, settings = S.current })
end)

RegisterNUICallback('reset', function(_, cb)
    cb({ settings = S.reset() })
end)

RegisterNUICallback('import', function(data, cb)
    local values = tableArg(data, 'settings')
    if type(values) ~= 'table' then
        cb({ ok = false, settings = S.current })
        return
    end
    cb({ ok = true, settings = S.apply(values) })
end)

RegisterNUICallback('test', function(data, cb)
    local kind = tableArg(data, 'kind')
    if kind == 'dummy' then
        -- Spawning waits for the model, so answer once it is done.
        CreateThread(function()
            Studio.test(kind)
            cb({ ok = true, dummy = Dummy.exists() })
        end)
        return
    end
    Studio.test(kind)
    cb({ ok = true, dummy = Dummy.exists() })
end)

RegisterNUICallback('painter', function(_, cb)
    Studio.close()
    BFX.Painter.request()
    cb({ ok = true })
end)

function Studio.shutdown()
    if Studio.state ~= 'closed' then SetNuiFocus(false, false) end
    Studio.state = 'closed'
    Dummy.remove()
end
