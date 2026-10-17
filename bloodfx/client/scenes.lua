-- Painted blood from the server. Only the nearest pieces are drawn, and only near the player.
local U = BFX.Util
local Decals = BFX.Decals

local Scenes = {
    items = {},   -- id -> item from the server
    handles = {}, -- id -> decal handle
    retry = {},   -- id -> earliest time to recreate a decal the game dropped
    own = {},     -- ids this player placed, newest last (undo)
}
BFX.Scenes = Scenes

local cfg = Config.Scenes

local function spawn(item)
    local r, g, b = U.hexToRgb(item.c)
    if not r then r, g, b = 0.23, 0.0, 0.0 end
    return Decals.create({
        type = item.d, x = item.x, y = item.y, z = item.z, nx = item.nx, ny = item.ny, nz = item.nz,
        w = item.s, h = item.s * (item.h or 1.0), angle = item.a, r = r, g = g, b = b, a = item.o or 1.0,
    })
end

function Scenes.remove(id)
    local handle = Scenes.handles[id]
    if handle then Decals.destroy(handle) end
    Scenes.handles[id] = nil
    Scenes.retry[id] = nil
    Scenes.items[id] = nil
end

function Scenes.clear()
    for _, handle in pairs(Scenes.handles) do Decals.destroy(handle) end
    Scenes.handles, Scenes.retry = {}, {}
end

function Scenes.popOwn()
    while #Scenes.own > 0 do
        local id = table.remove(Scenes.own)
        if Scenes.items[id] then return id end
    end
    return nil
end

function Scenes.refresh()
    local ped = PlayerPedId()
    local pos = GetEntityCoords(ped)
    local nearby = {}
    for id, item in pairs(Scenes.items) do
        local d = U.dist3(pos.x, pos.y, pos.z, item.x, item.y, item.z)
        if d <= cfg.streamDistance then nearby[#nearby + 1] = { id = id, d = d } end
    end
    table.sort(nearby, function(a, b) return a.d < b.d end)
    local wanted = {}
    for i = 1, math.min(#nearby, cfg.maxVisible) do wanted[nearby[i].id] = true end

    for id, handle in pairs(Scenes.handles) do
        if not wanted[id] then
            Decals.destroy(handle)
            Scenes.handles[id] = nil
            Scenes.retry[id] = nil
        elseif not IsDecalAlive(handle) then
            Scenes.handles[id] = nil -- the game dropped it; recreated below
        end
    end

    -- Decals only stick to collision that is loaded.
    if not HasCollisionLoadedAroundEntity(ped) then return end
    local now = GetGameTimer()
    for id in pairs(wanted) do
        if not Scenes.handles[id] and (Scenes.retry[id] or 0) <= now then
            Scenes.handles[id] = spawn(Scenes.items[id])
            Scenes.retry[id] = now + 20000
        end
    end
end

RegisterNetEvent('bloodfx:cl:scenes', function(list)
    Scenes.clear()
    Scenes.items = {}
    for _, item in ipairs(type(list) == 'table' and list or {}) do
        if type(item) == 'table' and item.id then Scenes.items[item.id] = item end
    end
    Scenes.refresh()
end)

RegisterNetEvent('bloodfx:cl:sceneAdded', function(item)
    if type(item) ~= 'table' or not item.id then return end
    Scenes.items[item.id] = item
    if item.by == GetPlayerServerId(PlayerId()) then
        Scenes.own[#Scenes.own + 1] = item.id
    end
    Scenes.refresh()
end)

RegisterNetEvent('bloodfx:cl:scenesRemoved', function(ids)
    for _, id in ipairs(type(ids) == 'table' and ids or {}) do
        Scenes.remove(id)
    end
end)

-- Someone cleaned an area: drop local effect blood there too.
RegisterNetEvent('bloodfx:cl:cleanArea', function(x, y, z, radius)
    if not (U.isFinite(x) and U.isFinite(y) and U.isFinite(z) and U.isFinite(radius)) then return end
    radius = U.clamp(radius, 0.0, 100.0)
    Decals.clearNear(x, y, z, radius)
    BFX.Peds.clearNear(x, y, z, radius)
    RemoveDecalsInRange(x, y, z, radius)
end)

CreateThread(function()
    while true do
        Wait(1500)
        if next(Scenes.items) or next(Scenes.handles) then Scenes.refresh() end
    end
end)
