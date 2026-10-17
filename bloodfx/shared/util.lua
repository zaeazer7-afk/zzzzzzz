BFX = BFX or {}

local U = {}
BFX.Util = U

local floor, sqrt, abs, min = math.floor, math.sqrt, math.abs, math.min

-- ------------------------------------------------------------------ numbers

function U.clamp(v, lo, hi)
    if v < lo then return lo end
    if v > hi then return hi end
    return v
end

function U.lerp(a, b, t)
    return a + (b - a) * t
end

function U.randf(lo, hi)
    return lo + math.random() * (hi - lo)
end

function U.round(v, decimals)
    local m = 10 ^ (decimals or 0)
    return floor(v * m + 0.5) / m
end

function U.isFinite(v)
    return type(v) == 'number' and v == v and v ~= math.huge and v ~= -math.huge
end

-- Natives return some BOOL out-values as booleans and others as 0/1 integers.
function U.truthy(v)
    return v == true or (type(v) == 'number' and v ~= 0)
end

-- Hashes arrive signed or unsigned depending on the source; compare them unsigned.
function U.u32(v)
    v = math.tointeger(v)
    if not v then return 0 end
    return v & 0xFFFFFFFF
end

-- Health as 0..1, treating 100 as dead the way GTA does for peds with more than 100 max health.
function U.healthRatio(health, maxHealth)
    if maxHealth > 100 then
        return U.clamp((health - 100) / (maxHealth - 100), 0.0, 1.0)
    end
    if maxHealth <= 0 then return 1.0 end
    return U.clamp(health / maxHealth, 0.0, 1.0)
end

-- ------------------------------------------------------------------ vectors

function U.dist3(ax, ay, az, bx, by, bz)
    local dx, dy, dz = ax - bx, ay - by, az - bz
    return sqrt(dx * dx + dy * dy + dz * dz)
end

function U.dist2(ax, ay, bx, by)
    local dx, dy = ax - bx, ay - by
    return sqrt(dx * dx + dy * dy)
end

-- Returns the unit vector and the original length.
function U.normalize(x, y, z)
    local len = sqrt(x * x + y * y + z * z)
    if len < 1e-6 then return 0.0, 0.0, 0.0, 0.0 end
    return x / len, y / len, z / len, len
end

-- Euler rotation in degrees that points an effect's local +Z axis along a direction.
function U.rotationZ(dx, dy, dz)
    local pitch = math.deg(math.acos(U.clamp(dz, -1.0, 1.0)))
    local yaw = math.deg(math.atan(dx, -dy))
    return pitch, 0.0, yaw
end

-- Same, for effects authored along their local +Y axis.
function U.rotationY(dx, dy, dz)
    local pitch = math.deg(math.asin(U.clamp(dz, -1.0, 1.0)))
    local yaw = math.deg(math.atan(-dx, dy))
    return pitch, 0.0, yaw
end

-- Two unit vectors spanning the plane perpendicular to the normal n.
function U.tangentBasis(nx, ny, nz)
    local ax, ay, az
    if abs(nz) < 0.9 then
        ax, ay, az = U.normalize(ny, -nx, 0.0)
    else
        ax, ay, az = U.normalize(-nz, 0.0, nx)
    end
    local bx = ny * az - nz * ay
    local by = nz * ax - nx * az
    local bz = nx * ay - ny * ax
    return ax, ay, az, bx, by, bz
end

-- A direction lying on the surface, rotated `angle` radians around its normal.
function U.surfaceVector(nx, ny, nz, angle)
    local ax, ay, az, bx, by, bz = U.tangentBasis(nx, ny, nz)
    local c, s = math.cos(angle), math.sin(angle)
    return ax * c + bx * s, ay * c + by * s, az * c + bz * s
end

-- The angle U.surfaceVector needs to point along `d` projected onto the surface.
function U.surfaceAngle(nx, ny, nz, dx, dy, dz)
    local ax, ay, az, bx, by, bz = U.tangentBasis(nx, ny, nz)
    local a = dx * ax + dy * ay + dz * az
    local b = dx * bx + dy * by + dz * bz
    if abs(a) < 1e-6 and abs(b) < 1e-6 then return nil end
    return math.atan(b, a)
end

-- ------------------------------------------------------------------ colours

function U.hexToRgb(hex)
    if type(hex) ~= 'string' then return nil end
    local h = hex:match('^%s*#?(%x+)%s*$')
    if not h then return nil end
    if #h == 3 then
        h = h:sub(1, 1):rep(2) .. h:sub(2, 2):rep(2) .. h:sub(3, 3):rep(2)
    end
    if #h ~= 6 then return nil end
    return tonumber(h:sub(1, 2), 16) / 255, tonumber(h:sub(3, 4), 16) / 255, tonumber(h:sub(5, 6), 16) / 255
end

function U.rgbToHex(r, g, b)
    local function byte(v) return floor(U.clamp(v, 0.0, 1.0) * 255 + 0.5) end
    return ('#%02x%02x%02x'):format(byte(r), byte(g), byte(b))
end

function U.normalizeHex(hex)
    local r, g, b = U.hexToRgb(hex)
    if not r then return nil end
    return U.rgbToHex(r, g, b)
end

function U.hsvToRgb(h, s, v)
    h = (h % 1.0) * 6.0
    local i = floor(h)
    local f = h - i
    local p, q, t = v * (1 - s), v * (1 - s * f), v * (1 - s * (1 - f))
    if i == 0 then return v, t, p end
    if i == 1 then return q, v, p end
    if i == 2 then return p, v, t end
    if i == 3 then return p, q, v end
    if i == 4 then return t, p, v end
    return v, p, q
end

-- ------------------------------------------------------------------ misc

function U.copy(t)
    local out = {}
    for k, v in pairs(t) do out[k] = v end
    return out
end

-- Token bucket used to cap how many effects can be created per second.
function U.newBucket(rate, capacity)
    return { rate = rate, cap = capacity or rate, tokens = capacity or rate, last = nil }
end

function U.takeToken(bucket, now, n)
    n = n or 1
    if bucket.last then
        bucket.tokens = min(bucket.cap, bucket.tokens + (now - bucket.last) * bucket.rate / 1000)
    end
    bucket.last = now
    if bucket.tokens >= n then
        bucket.tokens = bucket.tokens - n
        return true
    end
    return false
end

function U.debug(fmt, ...)
    if Config and Config.Debug then
        print(('[bloodfx] ' .. fmt):format(...))
    end
end
