-- Every player setting, its type and its range. Lua uses this to validate settings;
-- the studio UI uses it to build its controls.
local U = BFX.Util

local Schema = {}
BFX.Schema = Schema

Schema.version = 1

Schema.sections = {
    { id = 'look', label = 'Look' },
    { id = 'hits', label = 'Hits' },
    { id = 'splatter', label = 'Splatter' },
    { id = 'pools', label = 'Pools' },
    { id = 'screen', label = 'Screen' },
}

local SCOPES = {
    { value = 'all', label = 'Every hit near me' },
    { value = 'mine', label = 'Only hits I land' },
}

-- `preset = true` marks the "look" fields that a preset resets before applying its own values.
Schema.fields = {
    { key = 'enabled', section = 'look', type = 'bool', default = true, label = 'BloodFX enabled' },
    { key = 'color', section = 'look', type = 'color', default = '#5e0000', preset = true, label = 'Blood colour' },
    { key = 'rainbow', section = 'look', type = 'bool', default = false, preset = true, label = 'Rainbow cycle',
      hint = 'Cycles through every hue and overrides the colour above.' },
    { key = 'rainbowSpeed', section = 'look', type = 'number', default = 1.0, min = 0.1, max = 5.0, step = 0.1, unit = 'x',
      preset = true, depends = 'rainbow', label = 'Rainbow speed' },
    { key = 'tint', section = 'look', type = 'bool', default = false, preset = true, label = 'Tint particles',
      hint = 'Also colours particle effects. Only some game effects accept a tint; decals and screen blood always use your colour.' },
    { key = 'opacity', section = 'look', type = 'number', default = 0.95, min = 0.2, max = 1.0, step = 0.05, unit = '%',
      preset = true, label = 'Decal opacity' },

    { key = 'hit.enabled', section = 'hits', type = 'bool', default = true, label = 'Hit particles' },
    { key = 'hit.style', section = 'hits', type = 'select', default = 'realistic', options = 'styles', preset = true,
      depends = 'hit.enabled', label = 'Effect style' },
    { key = 'hit.scope', section = 'hits', type = 'select', default = 'all', options = SCOPES, label = 'Show effects for',
      hint = 'Only hits I land is the classic PvP setup.' },
    { key = 'hit.scale', section = 'hits', type = 'number', default = 1.0, min = 0.25, max = 3.0, step = 0.05, unit = 'x',
      preset = true, depends = 'hit.enabled', label = 'Particle size' },
    { key = 'hit.headshot', section = 'hits', type = 'number', default = 1.6, min = 1.0, max = 3.0, step = 0.1, unit = 'x',
      preset = true, label = 'Headshot boost' },
    { key = 'hit.kill', section = 'hits', type = 'bool', default = true, depends = 'hit.enabled', label = 'Kill burst',
      hint = 'Extra burst when a hit is fatal.' },
    { key = 'hit.splashback', section = 'hits', type = 'bool', default = true, label = 'Splashback',
      hint = 'Close-range hits splash blood onto your own clothes.' },

    { key = 'splatter.enabled', section = 'splatter', type = 'bool', default = true, label = 'Splatter decals' },
    { key = 'splatter.wall', section = 'splatter', type = 'bool', default = true, depends = 'splatter.enabled',
      label = 'Wall spray', hint = 'Blood sprays onto walls and floors behind the target.' },
    { key = 'splatter.ground', section = 'splatter', type = 'bool', default = true, depends = 'splatter.enabled',
      label = 'Ground drops' },
    { key = 'splatter.chance', section = 'splatter', type = 'number', default = 0.7, min = 0.0, max = 1.0, step = 0.05,
      unit = '%', preset = true, depends = 'splatter.enabled', label = 'Spray chance' },
    { key = 'splatter.size', section = 'splatter', type = 'number', default = 1.0, min = 0.3, max = 3.0, step = 0.05,
      unit = 'x', preset = true, depends = 'splatter.enabled', label = 'Splatter size' },
    { key = 'splatter.reach', section = 'splatter', type = 'number', default = 4.0, min = 1.0, max = 10.0, step = 0.5,
      unit = 'm', preset = true, depends = 'splatter.enabled', label = 'Spray reach' },
    { key = 'splatter.lifetime', section = 'splatter', type = 'number', default = 180, min = 10, max = 900, step = 10,
      unit = 's', label = 'Decal lifetime' },

    { key = 'pools.enabled', section = 'pools', type = 'bool', default = true, label = 'Death pools' },
    { key = 'pools.npcs', section = 'pools', type = 'bool', default = true, depends = 'pools.enabled', label = 'Pools under NPCs' },
    { key = 'pools.size', section = 'pools', type = 'number', default = 1.4, min = 0.4, max = 3.0, step = 0.1, unit = 'm',
      preset = true, depends = 'pools.enabled', label = 'Pool size' },
    { key = 'pools.growTime', section = 'pools', type = 'number', default = 10, min = 1, max = 30, step = 1, unit = 's',
      depends = 'pools.enabled', label = 'Spread time' },
    { key = 'bleed.enabled', section = 'pools', type = 'bool', default = true, label = 'Bleeding trails',
      hint = 'Wounded peds leave drops behind them.' },
    { key = 'bleed.threshold', section = 'pools', type = 'number', default = 0.5, min = 0.1, max = 0.95, step = 0.05,
      unit = '%', depends = 'bleed.enabled', label = 'Bleed below health' },
    { key = 'bleed.npcs', section = 'pools', type = 'bool', default = true, depends = 'bleed.enabled', label = 'NPCs bleed' },

    { key = 'screen.enabled', section = 'screen', type = 'bool', default = true, label = 'Screen blood',
      hint = 'Blood hits your screen when you take damage.' },
    { key = 'screen.intensity', section = 'screen', type = 'number', default = 0.8, min = 0.1, max = 1.5, step = 0.05,
      unit = 'x', preset = true, depends = 'screen.enabled', label = 'Intensity' },
    { key = 'screen.duration', section = 'screen', type = 'number', default = 2.5, min = 0.5, max = 8.0, step = 0.25,
      unit = 's', depends = 'screen.enabled', label = 'Duration' },
    { key = 'screen.lowHealth', section = 'screen', type = 'bool', default = true, depends = 'screen.enabled',
      label = 'Low-health pulse' },
    { key = 'screen.shake', section = 'screen', type = 'number', default = 0.0, min = 0.0, max = 1.0, step = 0.05,
      unit = '%', depends = 'screen.enabled', label = 'Camera shake' },
}

Schema.byKey = {}
for _, field in ipairs(Schema.fields) do
    Schema.byKey[field.key] = field
end

-- Server owners can tighten ranges in Config.Limits.
for key, limit in pairs(Config.Limits or {}) do
    local field = Schema.byKey[key]
    if field and field.type == 'number' and type(limit) == 'table' then
        field.min = tonumber(limit.min) or field.min
        field.max = tonumber(limit.max) or field.max
        if field.max < field.min then field.max = field.min end
    end
end

-- { hit = { scale = 1 } } -> { ['hit.scale'] = 1 }
function Schema.flatten(tbl, prefix, out)
    out = out or {}
    for k, v in pairs(tbl or {}) do
        local key = prefix and (prefix .. '.' .. tostring(k)) or tostring(k)
        if type(v) == 'table' and v[1] == nil and next(v) ~= nil then
            Schema.flatten(v, key, out)
        else
            out[key] = v
        end
    end
    return out
end

function Schema.options(field)
    if field.options == 'styles' then
        local list = {}
        for _, style in ipairs(Config.HitStyles or {}) do
            list[#list + 1] = { value = style.key, label = style.label or style.key }
        end
        return list
    end
    return field.options
end

function Schema.isLocked(key)
    if Config.AllowPlayerCustomization == false then return true end
    for _, lock in ipairs(Config.LockedSettings or {}) do
        if key == lock or key:sub(1, #lock + 1) == lock .. '.' then return true end
    end
    return false
end

-- Converts a raw value to the field's type, or returns nil when it is unusable.
function Schema.coerce(field, value)
    if value == nil then return nil end
    local kind = field.type
    if kind == 'bool' then
        if type(value) == 'boolean' then return value end
        if value == 1 or value == 0 then return value == 1 end
        return nil
    elseif kind == 'number' then
        value = tonumber(value)
        if not U.isFinite(value) then return nil end
        value = U.clamp(value, field.min, field.max)
        if field.step and field.step > 0 then
            value = field.min + math.floor((value - field.min) / field.step + 0.5) * field.step
            value = U.clamp(value, field.min, field.max)
        end
        return U.round(value, 4)
    elseif kind == 'select' then
        for _, option in ipairs(Schema.options(field) or {}) do
            if option.value == value then return value end
        end
        return nil
    elseif kind == 'color' then
        return U.normalizeHex(value)
    end
    return nil
end

local defaultsCache

function Schema.defaults()
    if defaultsCache then return defaultsCache end
    local flat = Schema.flatten(Config.Defaults or {})
    local out = {}
    for _, field in ipairs(Schema.fields) do
        local v = Schema.coerce(field, flat[field.key])
        if v == nil then v = Schema.coerce(field, field.default) end
        if v == nil and field.type == 'select' then
            local options = Schema.options(field)
            v = options and options[1] and options[1].value
        end
        out[field.key] = v
    end
    defaultsCache = out
    return out
end

-- Returns a complete, valid settings table. Unusable values fall back to `fallback`,
-- then to the defaults. Locked fields always use the defaults.
function Schema.sanitize(input, fallback)
    local defaults = Schema.defaults()
    local out = {}
    for _, field in ipairs(Schema.fields) do
        local v
        if not Schema.isLocked(field.key) then
            v = Schema.coerce(field, input and input[field.key])
            if v == nil and fallback then v = Schema.coerce(field, fallback[field.key]) end
        end
        if v == nil then v = defaults[field.key] end
        out[field.key] = v
    end
    return out
end

function Schema.preset(key)
    for _, preset in ipairs(Config.Presets or {}) do
        if preset.key == key then return preset end
    end
    return nil
end

function Schema.applyPreset(current, preset)
    local defaults = Schema.defaults()
    local merged = U.copy(current)
    for _, field in ipairs(Schema.fields) do
        if field.preset then merged[field.key] = defaults[field.key] end
    end
    for k, v in pairs(Schema.flatten(preset.values or {})) do
        merged[k] = v
    end
    return Schema.sanitize(merged, current)
end

-- Field list for the UI, with options resolved and lock state included.
function Schema.export()
    local list = {}
    for i, field in ipairs(Schema.fields) do
        list[i] = {
            key = field.key, section = field.section, type = field.type, label = field.label, hint = field.hint,
            min = field.min, max = field.max, step = field.step, unit = field.unit, depends = field.depends,
            options = Schema.options(field), locked = Schema.isLocked(field.key),
        }
    end
    return list
end

function Schema.presetCards()
    local defaults = Schema.defaults()
    local cards = {}
    for i, preset in ipairs(Config.Presets or {}) do
        local values = Schema.flatten(preset.values or {})
        cards[i] = {
            key = preset.key,
            label = preset.label or preset.key,
            color = U.normalizeHex(values.color) or defaults.color,
            rainbow = values.rainbow == true,
        }
    end
    return cards
end
