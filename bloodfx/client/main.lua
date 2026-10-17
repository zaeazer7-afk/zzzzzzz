-- Start-up, commands, exports and clean-up.
local U = BFX.Util
local S = BFX.Settings

function BFX.notify(message)
    BeginTextCommandThefeedPost('STRING')
    AddTextComponentSubstringPlayerName(message)
    EndTextCommandThefeedPostTicker(false, true)
end

RegisterNetEvent('bloodfx:cl:notify', function(message)
    if type(message) == 'string' then BFX.notify(message) end
end)

CreateThread(function()
    math.randomseed(GetGameTimer())
    BFX.Fx.preload()
    while not NetworkIsPlayerActive(PlayerId()) do Wait(250) end
    TriggerServerEvent('bloodfx:sv:requestScenes')
end)

-- ------------------------------------------------------------------ commands

local commands = Config.Commands or {}

local function suggest(name, help, params)
    TriggerEvent('chat:addSuggestion', '/' .. name, help, params or {})
end

if commands.studio then
    RegisterCommand(commands.studio, function() BFX.Studio.toggle() end, false)
    RegisterKeyMapping(commands.studio, 'Open BloodFX Studio', 'keyboard', Config.OpenKey or '')
    suggest(commands.studio, 'Open BloodFX Studio')
end

if commands.painter then
    RegisterCommand(commands.painter, function() BFX.Painter.request() end, false)
    suggest(commands.painter, 'Toggle the crime-scene blood painter')
end

if commands.clean then
    RegisterCommand(commands.clean, function(_, args)
        local radius = U.clamp(tonumber(args[1]) or 10.0, 1.0, 100.0)
        local p = GetEntityCoords(PlayerPedId())
        TriggerServerEvent('bloodfx:sv:clean', radius, { x = p.x, y = p.y, z = p.z })
    end, false)
    suggest(commands.clean, 'Remove blood near you', { { name = 'radius', help = 'metres (default 10)' } })
end

if commands.cleanSelf then
    RegisterCommand(commands.cleanSelf, function()
        local me = PlayerPedId()
        ClearPedBloodDamage(me)
        ResetPedVisibleDamage(me)
        BFX.notify(Config.Text.cleanedSelf)
    end, false)
    suggest(commands.cleanSelf, 'Wipe the blood off your character')
end

-- ------------------------------------------------------------------ exports

local function xyz(v)
    if type(v) ~= 'table' and type(v) ~= 'vector3' and type(v) ~= 'vector4' then return nil end
    local x, y, z = tonumber(v.x or v[1]), tonumber(v.y or v[2]), tonumber(v.z or v[3])
    if not (U.isFinite(x) and U.isFinite(y) and U.isFinite(z)) then return nil end
    return x, y, z
end

exports('OpenStudio', function() BFX.Studio.open() end)
exports('GetSettings', function() return U.copy(S.current) end)
exports('SetSettings', function(values) return U.copy(S.apply(values)) end)

-- exports.bloodfx:HitEffect(ped, { weapon = `WEAPON_PISTOL`, headshot = false, fatal = false })
exports('HitEffect', function(ped, opts)
    opts = type(opts) == 'table' and opts or {}
    if not ped or not DoesEntityExist(ped) or not IsEntityAPed(ped) then return false end
    local kind = BFX.Hits.classify(opts.weapon or 0) or 'bullet'
    local c = GetPedBoneCoords(ped, opts.headshot and 31086 or 24818, 0.0, 0.0, 0.0)
    local cam = GetFinalRenderedCamCoord()
    local dx, dy, dz = U.normalize(c.x - cam.x, c.y - cam.y, c.z - cam.z)
    BFX.Fx.hit(ped, c.x, c.y, c.z, dx, dy, dz, {
        kind = kind, spec = BFX.Hits.kinds[kind], headshot = opts.headshot == true, fatal = opts.fatal == true,
        inVehicle = IsPedInAnyVehicle(ped, false), inWater = IsEntityInWater(ped),
    })
    return true
end)

-- exports.bloodfx:Splatter(vector3(x, y, z), 0.8, '#5e0000')
exports('Splatter', function(coords, size, color)
    local x, y, z = xyz(coords)
    if not x then return false end
    local r, g, b
    if color then r, g, b = U.hexToRgb(color) end
    return BFX.Fx.splatterAt(x, y, z, tonumber(size) or 0.8, r, g, b)
end)

-- exports.bloodfx:Pool(vector3(x, y, z), 1.5)
exports('Pool', function(coords, size)
    local x, y, z = xyz(coords)
    if not x then return false end
    local hit, gx, gy, gz, nx, ny, nz = BFX.Fx.ground(x, y, z, 0, 4.0)
    if not hit then return false end
    BFX.Peds.startPool(nil, gx, gy, gz, nx, ny, nz, tonumber(size))
    return true
end)

exports('ScreenBlood', function(intensity, color)
    SendNUIMessage({
        type = 'screen:hit',
        color = U.normalizeHex(color) or S.hex(),
        intensity = U.clamp(tonumber(intensity) or 1.0, 0.1, 2.0),
        duration = S.get('screen.duration'),
        headshot = false,
    })
end)

exports('ClearArea', function(coords, radius)
    local x, y, z = xyz(coords)
    if not x then return 0 end
    radius = U.clamp(tonumber(radius) or 10.0, 0.5, 200.0)
    BFX.Peds.clearNear(x, y, z, radius)
    return BFX.Decals.clearNear(x, y, z, radius)
end)

-- ------------------------------------------------------------------ clean-up

AddEventHandler('onResourceStop', function(resource)
    if resource ~= GetCurrentResourceName() then return end
    BFX.Studio.shutdown()
    BFX.Painter.stop()
    BFX.Peds.clear()
    BFX.Scenes.clear()
    BFX.Decals.clearAll()
    BFX.Fx.resetNativeBlood()
end)
