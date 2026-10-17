-- Stores painted blood, checks every request and keeps all clients in sync.
local U = BFX.Util

local RESOURCE = GetCurrentResourceName()
local FILE = 'data/scenes.json'

local cfg = Config.Scenes
local painterCfg = Config.Painter

local scenes = {}  -- id -> item
local count = 0
local nextId = 1
local saveQueued = false
local buckets = {} -- source -> token bucket

local kinds = {}
for _, kind in ipairs(painterCfg.kinds) do kinds[kind.key] = kind end

local function notify(source, message)
    TriggerClientEvent('bloodfx:cl:notify', source, message)
end

-- ------------------------------------------------------------------ persistence

local function sortedList()
    local list = {}
    for _, item in pairs(scenes) do list[#list + 1] = item end
    table.sort(list, function(a, b) return a.id < b.id end)
    return list
end

local function save()
    saveQueued = false
    if not cfg.persist then return end
    SaveResourceFile(RESOURCE, FILE, json.encode(sortedList()), -1)
end

local function queueSave()
    if saveQueued or not cfg.persist then return end
    saveQueued = true
    SetTimeout(5000, save)
end

local function finite(...)
    for i = 1, select('#', ...) do
        if not U.isFinite(select(i, ...)) then return false end
    end
    return true
end

local function validStored(item)
    return type(item) == 'table' and math.type(item.id) == 'integer' and kinds[item.k] ~= nil
        and finite(item.x, item.y, item.z, item.nx, item.ny, item.nz, item.s, item.a)
end

local function load()
    if not cfg.persist then return end
    local raw = LoadResourceFile(RESOURCE, FILE)
    if not raw or raw == '' then return end
    local ok, data = pcall(json.decode, raw)
    if not ok or type(data) ~= 'table' then
        print(('[bloodfx] %s is not valid JSON; starting with no painted blood'):format(FILE))
        return
    end
    for _, item in ipairs(data) do
        if validStored(item) then
            item.d = kinds[item.k].decal -- follow config changes
            item.by = 0                  -- server ids from an earlier session mean nothing now
            scenes[item.id] = item
            count = count + 1
            if item.id >= nextId then nextId = item.id + 1 end
        end
    end
    print(('[bloodfx] restored %d painted blood decals'):format(count))
end

-- ------------------------------------------------------------------ scene store

local function removeIds(ids)
    if #ids == 0 then return end
    for _, id in ipairs(ids) do
        if scenes[id] then
            scenes[id] = nil
            count = count - 1
        end
    end
    TriggerClientEvent('bloodfx:cl:scenesRemoved', -1, ids)
    queueSave()
end

local function insert(item)
    item.id = nextId
    nextId = nextId + 1
    scenes[item.id] = item
    count = count + 1
    -- Over the cap: drop the oldest.
    if count > cfg.maxTotal then
        local list = sortedList()
        local drop = {}
        for i = 1, count - cfg.maxTotal do drop[#drop + 1] = list[i].id end
        removeIds(drop)
    end
    TriggerClientEvent('bloodfx:cl:sceneAdded', -1, item)
    queueSave()
    return item.id
end

local function removeNear(x, y, z, radius)
    local ids = {}
    for id, item in pairs(scenes) do
        if U.dist3(x, y, z, item.x, item.y, item.z) <= radius then ids[#ids + 1] = id end
    end
    removeIds(ids)
    return #ids
end

-- Builds a clean item from untrusted input, or returns nil.
local function buildItem(data, by)
    if type(data) ~= 'table' then return nil end
    local kind = kinds[data.k]
    if not kind then return nil end
    local x, y, z = tonumber(data.x), tonumber(data.y), tonumber(data.z)
    local nx, ny, nz = tonumber(data.nx), tonumber(data.ny), tonumber(data.nz)
    local size, angle = tonumber(data.s), tonumber(data.a) or 0.0
    if not finite(x, y, z, nx, ny, nz, size, angle) then return nil end
    if math.abs(x) > 20000.0 or math.abs(y) > 20000.0 or math.abs(z) > 5000.0 then return nil end

    local len = math.sqrt(nx * nx + ny * ny + nz * nz)
    if len < 0.01 then return nil end
    nx, ny, nz = nx / len, ny / len, nz / len

    local colour = (painterCfg.color and U.normalizeHex(painterCfg.color)) or U.normalizeHex(data.c) or '#3a0000'
    return {
        k = kind.key, d = kind.decal, h = kind.aspect or 1.0,
        x = U.round(x, 3), y = U.round(y, 3), z = U.round(z, 3),
        nx = U.round(nx, 4), ny = U.round(ny, 4), nz = U.round(nz, 4),
        s = U.round(U.clamp(size, painterCfg.minSize, painterCfg.maxSize), 3),
        a = U.round(angle % (math.pi * 2.0), 4),
        c = colour,
        o = U.round(U.clamp(tonumber(data.o) or 1.0, 0.1, 1.0), 2),
        by = by,
        t = os.time(),
    }
end

-- Player position from the server's point of view (OneSync), or nil without it.
local function playerCoords(source)
    local ped = GetPlayerPed(tostring(source))
    if not ped or ped == 0 then return nil end
    local p = GetEntityCoords(ped)
    if p.x == 0.0 and p.y == 0.0 and p.z == 0.0 then return nil end
    return p
end

-- ------------------------------------------------------------------ events

RegisterNetEvent('bloodfx:sv:requestScenes', function()
    TriggerClientEvent('bloodfx:cl:scenes', source, sortedList())
end)

RegisterNetEvent('bloodfx:sv:painterAccess', function(probe)
    local src = source
    TriggerClientEvent('bloodfx:cl:painterAccess', src, SvConfig.CanPaint(src) == true, probe == true)
end)

RegisterNetEvent('bloodfx:sv:addScene', function(data)
    local src = source
    if not SvConfig.CanPaint(src) then
        U.debug('player %s tried to paint without permission', src)
        return
    end
    local bucket = buckets[src]
    if not bucket then
        bucket = U.newBucket(cfg.maxPerSecond, cfg.maxPerSecond)
        buckets[src] = bucket
    end
    if not U.takeToken(bucket, GetGameTimer()) then return end

    local item = buildItem(data, src)
    if not item then return end
    local p = playerCoords(src)
    if p and U.dist3(p.x, p.y, p.z, item.x, item.y, item.z) > cfg.maxPlaceDistance then
        U.debug('player %s painted too far away', src)
        return
    end
    insert(item)
end)

RegisterNetEvent('bloodfx:sv:removeScene', function(id)
    local src = source
    local item = scenes[tonumber(id) or -1]
    if not item then return end
    if item.by ~= src and not SvConfig.CanClean(src) then return end
    removeIds({ item.id })
end)

RegisterNetEvent('bloodfx:sv:clean', function(radius, coords)
    local src = source
    if not SvConfig.CanClean(src) then
        notify(src, Config.Text.noPermission)
        return
    end
    radius = U.clamp(tonumber(radius) or 10.0, 1.0, 100.0)
    local p = playerCoords(src)
    local x, y, z
    if p then
        x, y, z = p.x, p.y, p.z
    elseif type(coords) == 'table' and finite(tonumber(coords.x), tonumber(coords.y), tonumber(coords.z)) then
        x, y, z = tonumber(coords.x), tonumber(coords.y), tonumber(coords.z)
    else
        return
    end
    removeNear(x, y, z, radius)
    TriggerClientEvent('bloodfx:cl:cleanArea', -1, x, y, z, radius)
    notify(src, Config.Text.cleaned:format(math.floor(radius + 0.5)))
end)

AddEventHandler('playerDropped', function()
    buckets[source] = nil
end)

-- Painted blood can expire.
CreateThread(function()
    while true do
        Wait(60000)
        if cfg.expireMinutes and cfg.expireMinutes > 0 then
            local cutoff = os.time() - cfg.expireMinutes * 60
            local ids = {}
            for id, item in pairs(scenes) do
                if (item.t or 0) < cutoff then ids[#ids + 1] = id end
            end
            removeIds(ids)
        end
    end
end)

AddEventHandler('onResourceStop', function(resource)
    if resource == RESOURCE and saveQueued then save() end
end)

RegisterCommand('bloodfx_clearall', function(source)
    local ids = {}
    for id in pairs(scenes) do ids[#ids + 1] = id end
    removeIds(ids)
    print(('[bloodfx] cleared %d painted blood decals'):format(#ids))
end, true)

-- ------------------------------------------------------------------ exports

local function xyz(v)
    if v == nil then return nil end
    local x, y, z = tonumber(v.x or v[1]), tonumber(v.y or v[2]), tonumber(v.z or v[3])
    if not finite(x, y, z) then return nil end
    return x, y, z
end

-- exports.bloodfx:AddScene(vector3(x, y, z), 'pool', 1.5, { color = '#3a0000', normal = vector3(0, 0, 1) })
exports('AddScene', function(coords, kind, size, opts)
    local x, y, z = xyz(coords)
    if not x then return nil end
    opts = type(opts) == 'table' and opts or {}
    local nx, ny, nz = 0.0, 0.0, 1.0
    if opts.normal then nx, ny, nz = xyz(opts.normal) end
    local k = kinds[kind] and kind or painterCfg.kinds[1].key
    local item = buildItem({
        k = k, x = x, y = y, z = z, nx = nx or 0.0, ny = ny or 0.0, nz = nz or 1.0,
        s = tonumber(size) or kinds[k].size, a = tonumber(opts.angle) or math.random() * math.pi * 2.0,
        c = opts.color, o = opts.opacity,
    }, 0)
    if not item then return nil end
    return insert(item)
end)

exports('RemoveScene', function(id)
    if not scenes[tonumber(id) or -1] then return false end
    removeIds({ tonumber(id) })
    return true
end)

exports('RemoveScenesNear', function(coords, radius)
    local x, y, z = xyz(coords)
    if not x then return 0 end
    return removeNear(x, y, z, U.clamp(tonumber(radius) or 10.0, 0.5, 500.0))
end)

exports('GetScenes', function()
    return sortedList()
end)

load()
