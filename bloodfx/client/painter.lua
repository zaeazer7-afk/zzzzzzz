-- /bloodpaint: place persistent, synced blood decals where you aim.
local U = BFX.Util
local S = BFX.Settings
local Fx = BFX.Fx
local Decals = BFX.Decals

local Painter = { active = false, kind = 1, sizes = {}, rotation = 0.0 }
BFX.Painter = Painter

local cfg = Config.Painter
local kinds = cfg.kinds

local CONTROL_PLACE = 24   -- INPUT_ATTACK (left mouse)
local CONTROL_KIND = 38    -- INPUT_PICKUP (E)
local CONTROL_BIGGER = 15  -- INPUT_WEAPON_WHEEL_PREV (scroll up)
local CONTROL_SMALLER = 14 -- INPUT_WEAPON_WHEEL_NEXT (scroll down)
local CONTROL_ROTATE = 45  -- INPUT_RELOAD (R)
local CONTROL_UNDO = 47    -- INPUT_DETONATE (G)
local CONTROL_EXIT = 194   -- INPUT_FRONTEND_RRIGHT (Backspace)

-- Weapon, melee and weapon-wheel inputs are blocked while painting.
local BLOCKED = { 14, 15, 16, 17, 24, 25, 37, 38, 45, 47, 99, 100, 140, 141, 142, 143, 257, 261, 262, 263, 264 }

local preview = { handle = nil, key = nil, at = 0 }
local lastPlace = 0

local function clearPreview()
    if preview.handle then Decals.destroy(preview.handle) end
    preview.handle, preview.key = nil, nil
end

-- A see-through copy of the decal under the crosshair, refreshed when something changes.
local function updatePreview(hit, x, y, z, nx, ny, nz, kind, size, angle, r, g, b)
    if not hit then
        clearPreview()
        return
    end
    local key = ('%.2f|%.2f|%.2f|%s|%.2f|%.2f'):format(x, y, z, kind.key, size, angle)
    local now = GetGameTimer()
    if key == preview.key or now - preview.at < 90 then return end
    local handle = Decals.create({
        type = kind.decal, x = x, y = y, z = z, nx = nx, ny = ny, nz = nz,
        w = size, h = size * (kind.aspect or 1.0), angle = angle, r = r, g = g, b = b, a = 0.5,
    })
    if preview.handle then Decals.destroy(preview.handle) end
    preview.handle, preview.key, preview.at = handle, key, now
end

local function cameraRay(range)
    local cam = GetFinalRenderedCamCoord()
    local rot = GetFinalRenderedCamRot(2)
    local rx, rz = math.rad(rot.x), math.rad(rot.z)
    local horizontal = math.abs(math.cos(rx))
    local dx, dy, dz = -math.sin(rz) * horizontal, math.cos(rz) * horizontal, math.sin(rx)
    local hit, x, y, z, nx, ny, nz = Fx.raycast(cam.x, cam.y, cam.z,
        cam.x + dx * range, cam.y + dy * range, cam.z + dz * range, PlayerPedId())
    return hit, x, y, z, nx, ny, nz, dx, dy, dz
end

local function colour()
    local forced = cfg.color and U.normalizeHex(cfg.color)
    if forced then return forced, U.hexToRgb(forced) end
    local r, g, b = S.rgb()
    return U.rgbToHex(r, g, b), r, g, b
end

local function helpText(kind, size)
    BeginTextCommandDisplayHelp('CELL_EMAIL_BCON')
    AddTextComponentSubstringPlayerName(('~INPUT_ATTACK~ Place ~r~%s~s~ (%.1f m)~n~'):format(kind.label, size))
    AddTextComponentSubstringPlayerName('~INPUT_PICKUP~ Type  ~INPUT_WEAPON_WHEEL_PREV~ ~INPUT_WEAPON_WHEEL_NEXT~ Size~n~')
    AddTextComponentSubstringPlayerName('~INPUT_RELOAD~ Rotate  ~INPUT_DETONATE~ Undo  ~INPUT_FRONTEND_RRIGHT~ Exit')
    EndTextCommandDisplayHelp(0, false, false, -1)
end

function Painter.undo()
    local id = BFX.Scenes.popOwn()
    if not id then
        BFX.notify(Config.Text.nothingToUndo)
        return
    end
    TriggerServerEvent('bloodfx:sv:removeScene', id)
end

function Painter.stop()
    if not Painter.active then return end
    Painter.active = false
    clearPreview()
    BFX.notify(Config.Text.painterOff)
end

function Painter.start()
    if Painter.active then return end
    Painter.active = true
    BFX.notify(Config.Text.painterOn)

    CreateThread(function()
        while Painter.active do
            for i = 1, #BLOCKED do DisableControlAction(0, BLOCKED[i], true) end

            local kind = kinds[Painter.kind]
            local size = Painter.sizes[kind.key] or kind.size
            local hit, x, y, z, nx, ny, nz, dx, dy, dz = cameraRay(cfg.range)
            local angle = Painter.rotation
            if hit and kind.directional then
                angle = (U.surfaceAngle(nx, ny, nz, dx, dy, dz) or 0.0) + Painter.rotation
            end
            local hex, r, g, b = colour()

            if hit then
                DrawMarker(28, x, y, z, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.05, 0.05, 0.05,
                    255, 255, 255, 170, false, false, 2, false, nil, nil, false)
            end
            updatePreview(hit, x, y, z, nx, ny, nz, kind, size, angle, r, g, b)
            helpText(kind, size)

            local now = GetGameTimer()
            if hit and IsDisabledControlJustPressed(0, CONTROL_PLACE) and now - lastPlace > 150 then
                lastPlace = now
                TriggerServerEvent('bloodfx:sv:addScene', {
                    k = kind.key, x = x, y = y, z = z, nx = nx, ny = ny, nz = nz,
                    s = size, a = angle, c = hex, o = S.get('opacity'),
                })
            end
            if IsDisabledControlJustPressed(0, CONTROL_KIND) then
                Painter.kind = Painter.kind % #kinds + 1
            end
            if IsDisabledControlJustPressed(0, CONTROL_BIGGER) then
                Painter.sizes[kind.key] = U.clamp(size * 1.12, cfg.minSize, cfg.maxSize)
            end
            if IsDisabledControlJustPressed(0, CONTROL_SMALLER) then
                Painter.sizes[kind.key] = U.clamp(size / 1.12, cfg.minSize, cfg.maxSize)
            end
            if IsDisabledControlJustPressed(0, CONTROL_ROTATE) then
                Painter.rotation = (Painter.rotation + math.pi / 8.0) % (math.pi * 2.0)
            end
            if IsDisabledControlJustPressed(0, CONTROL_UNDO) then
                Painter.undo()
            end
            if IsControlJustPressed(0, CONTROL_EXIT) or IsEntityDead(PlayerPedId()) then
                Painter.stop()
            end
            Wait(0)
        end
        clearPreview()
    end)
end

-- Asks the server; it answers with bloodfx:cl:painterAccess.
function Painter.request()
    if Painter.active then
        Painter.stop()
        return
    end
    TriggerServerEvent('bloodfx:sv:painterAccess', false)
end

RegisterNetEvent('bloodfx:cl:painterAccess', function(allowed, probe)
    if probe then
        BFX.Studio.setCanPaint(allowed == true)
    elseif allowed then
        Painter.start()
    else
        BFX.notify(Config.Text.noPermission)
    end
end)
