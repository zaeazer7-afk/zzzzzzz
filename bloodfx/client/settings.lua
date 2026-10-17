-- The local player's settings, validated by the schema and saved on their PC (resource KVP).
local U = BFX.Util
local Schema = BFX.Schema

local Settings = { current = {}, listeners = {} }
BFX.Settings = Settings

local KVP_KEY = 'bloodfx:settings'
local baseR, baseG, baseB = 0.37, 0.0, 0.0
local saveToken = 0

local function refreshColour()
    local r, g, b = U.hexToRgb(Settings.current.color)
    if r then baseR, baseG, baseB = r, g, b end
    for _, fn in ipairs(Settings.listeners) do fn(Settings.current) end
end

-- Runs `fn(settings)` now and after every change.
function Settings.onChange(fn)
    Settings.listeners[#Settings.listeners + 1] = fn
    fn(Settings.current)
end

local function queueSave()
    saveToken = saveToken + 1
    local token = saveToken
    SetTimeout(750, function()
        if token == saveToken then Settings.save() end
    end)
end

function Settings.get(key)
    return Settings.current[key]
end

function Settings.load()
    local saved
    local raw = GetResourceKvpString(KVP_KEY)
    if raw and raw ~= '' then
        local ok, decoded = pcall(json.decode, raw)
        if ok and type(decoded) == 'table' then
            saved = type(decoded.s) == 'table' and decoded.s or decoded
        end
    end
    Settings.current = Schema.sanitize(saved or {})
    refreshColour()
end

function Settings.save()
    SetResourceKvp(KVP_KEY, json.encode({ v = Schema.version, s = Settings.current }))
end

-- Merges a full or partial flat table ({ ['hit.scale'] = 1.2 }) and returns the clean result.
function Settings.apply(values)
    local merged = U.copy(Settings.current)
    if type(values) == 'table' then
        for k, v in pairs(values) do
            if type(k) == 'string' then merged[k] = v end
        end
    end
    Settings.current = Schema.sanitize(merged, Settings.current)
    refreshColour()
    queueSave()
    return Settings.current
end

function Settings.applyPreset(key)
    local preset = Schema.preset(key)
    if not preset then return nil end
    Settings.current = Schema.applyPreset(Settings.current, preset)
    refreshColour()
    queueSave()
    return Settings.current
end

function Settings.reset()
    Settings.current = Schema.sanitize({})
    refreshColour()
    queueSave()
    return Settings.current
end

-- Current blood colour as 0..1 floats; cycles when rainbow mode is on.
function Settings.rgb()
    local s = Settings.current
    if s.rainbow then
        local hue = (GetGameTimer() * 0.00025 * (s.rainbowSpeed or 1.0)) % 1.0
        return U.hsvToRgb(hue, 0.85, 1.0)
    end
    return baseR, baseG, baseB
end

function Settings.hex()
    return U.rgbToHex(Settings.rgb())
end

Settings.load()
