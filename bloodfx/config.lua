Config = {}

-- Prints diagnostics (missing particle effects, permission denials) to the F8 / server console.
Config.Debug = false

-- Chat commands. Set any of them to false to disable it.
Config.Commands = {
    studio = 'bloodfx',       -- opens BloodFX Studio
    painter = 'bloodpaint',   -- crime-scene painter, needs permission (server/sv_config.lua)
    clean = 'bloodclean',     -- removes painted blood near you, needs permission
    cleanSelf = 'cleanblood', -- wipes the blood off your own character
}

-- Default key for the studio. Players can rebind it in Settings > Key Bindings > FiveM.
-- Leave it empty to register the binding without a default key, e.g. set 'F7' to give it one.
Config.OpenKey = ''

-- When false, players can still open the studio and use the test buttons,
-- but every setting is locked to Config.Defaults.
Config.AllowPlayerCustomization = true

-- Settings players may not change. A section name locks the whole section.
-- Examples: { 'color', 'rainbow', 'tint' } forces realistic colours, { 'pools' } locks pool settings.
Config.LockedSettings = {}

-- Tighten slider ranges, e.g. { ['hit.scale'] = { max = 1.5 }, ['splatter.lifetime'] = { max = 300 } }
Config.Limits = {}

-- What every player starts with. Players' own changes are saved on their PC.
Config.Defaults = {
    enabled = true,
    color = '#5e0000',      -- blood colour (hex)
    rainbow = false,
    rainbowSpeed = 1.0,
    tint = false,           -- also tint particle effects with the colour
    opacity = 0.95,         -- decal opacity
    nativeBlood = 'normal', -- the game's own blood: 'normal', 'clown' (colourful clown hits) or 'alien'

    hit = {
        enabled = true,
        style = 'realistic', -- key from Config.HitStyles
        scope = 'all',       -- 'all' = every hit near you, 'mine' = only hits you land
        scale = 1.0,
        headshot = 1.6,      -- size multiplier on headshots
        kill = true,         -- extra burst on fatal hits
        splashback = true,   -- close-range hits splash blood on your clothes
    },

    splatter = {
        enabled = true,
        wall = true,         -- spray onto walls and floors behind the target
        ground = true,       -- drops on the ground around the target
        chance = 0.7,
        size = 1.0,
        reach = 4.0,         -- metres a spray can travel
        lifetime = 180,      -- seconds before a decal is removed
    },

    pools = {
        enabled = true,
        npcs = true,
        size = 1.4,          -- metres
        growTime = 10,       -- seconds to spread to full size
    },

    bleed = {
        enabled = true,
        threshold = 0.5,     -- peds bleed below this share of their health
        npcs = true,
    },

    screen = {
        enabled = true,
        intensity = 0.8,
        duration = 2.5,
        lowHealth = true,
        shake = 0.0,
    },
}

-- One-click looks in the studio. A preset resets the "look" settings to the defaults
-- and then applies its own values; gameplay toggles such as pools stay as they are.
Config.Presets = {
    { key = 'realistic', label = 'Realistic', values = {} },
    { key = 'vanilla', label = 'Vanilla+', values = {
        color = '#320000', hit = { style = 'subtle', scale = 0.8 },
        splatter = { chance = 0.4, size = 0.8 }, pools = { size = 1.0 },
    } },
    { key = 'gore', label = 'Cinematic', values = {
        color = '#6e0000', hit = { style = 'gore', scale = 1.3, headshot = 2.2 },
        splatter = { chance = 1.0, size = 1.6, reach = 6.5 }, pools = { size = 2.2 },
    } },
    { key = 'crimson', label = 'Crimson', values = {
        color = '#d40000', tint = true, hit = { style = 'pvp', scale = 1.2, headshot = 2.0 },
        splatter = { chance = 0.9, size = 1.2 },
    } },
    { key = 'neon', label = 'Neon', values = { color = '#ff1fd1', tint = true, hit = { style = 'pvp', scale = 1.2 } } },
    { key = 'ice', label = 'Ice', values = { color = '#1fb4ff', tint = true, hit = { style = 'pvp', scale = 1.2 } } },
    { key = 'toxic', label = 'Toxic', values = { color = '#5cff1f', tint = true, hit = { style = 'pvp', scale = 1.2 } } },
    { key = 'gold', label = 'Gold', values = { color = '#ffb21f', tint = true, hit = { style = 'pvp', scale = 1.2 } } },
    { key = 'void', label = 'Void', values = { color = '#7a2bff', tint = true, hit = { style = 'pvp', scale = 1.2 } } },
    { key = 'rainbow', label = 'Rainbow', values = {
        rainbow = true, rainbowSpeed = 1.5, tint = true, hit = { style = 'pvp', scale = 1.2 },
    } },
    { key = 'confetti', label = 'Confetti', values = { color = '#33b8ff', hit = { style = 'confetti' } } },
    { key = 'clown', label = 'Clown', values = { color = '#ff4fd8', nativeBlood = 'clown', hit = { style = 'confetti' } } },
    { key = 'gumballs', label = 'Gumballs', values = { color = '#ff7a1f', hit = { style = 'gumballs' } } },
    { key = 'cash', label = 'Cash', values = { color = '#1f8a3a', hit = { style = 'cash' } } },
}

--[[
    Particle styles for bullet hits. Every effect name below exists in the game files.
    Each layer:
      asset   particle dictionary ('core' is always loaded)
      names   effect names, tried in order until one starts
      scale   size multiplier
      dir     'back' sprays toward the shooter (entry wound), 'forward' along the bullet (exit),
              'up' points up with a random spin, 'none' keeps the effect's own rotation
      offset  metres to move the effect along the bullet direction
      chance  0-1, how often the layer plays
      axis    'z' (default) or 'y', the axis the effect was authored along
    Groups: hit (every hit), headshot (extra on headshots), kill (extra on fatal hits),
            melee / blade (optional, override Config.WeaponEffects for this style).
    headshotReplaces = true plays only the headshot layers on head and neck hits, the way
    bloodfx.dat packs swap headshots for confetti. A style with empty hit and headshot lists
    never spawns particles.
]]
Config.HitStyles = {
    {
        key = 'realistic', label = 'Realistic',
        hit = {
            { asset = 'core', names = { 'blood_entry' }, scale = 1.0, dir = 'back' },
            { asset = 'core', names = { 'blood_exit' }, scale = 1.0, dir = 'forward', offset = 0.12, chance = 0.6 },
        },
        headshot = {
            { asset = 'core', names = { 'blood_headshot', 'blood_entry_head_sniper' }, scale = 1.0, dir = 'forward', offset = 0.08 },
        },
        kill = {
            { asset = 'core', names = { 'blood_mist' }, scale = 0.9, dir = 'forward', offset = 0.1 },
        },
    },
    {
        key = 'subtle', label = 'Subtle',
        hit = {
            { asset = 'core', names = { 'blood_entry' }, scale = 0.7, dir = 'back' },
        },
        headshot = {
            { asset = 'core', names = { 'blood_headshot' }, scale = 0.8, dir = 'forward' },
        },
        kill = {},
    },
    {
        key = 'heavy', label = 'Heavy',
        hit = {
            { asset = 'core', names = { 'blood_entry_shotgun', 'blood_entry' }, scale = 1.1, dir = 'back' },
            { asset = 'core', names = { 'blood_exit' }, scale = 1.3, dir = 'forward', offset = 0.12 },
            { asset = 'core', names = { 'blood_mist' }, scale = 0.6, dir = 'forward', offset = 0.2, chance = 0.5 },
        },
        headshot = {
            { asset = 'core', names = { 'blood_entry_head_sniper', 'blood_headshot' }, scale = 1.3, dir = 'forward' },
            { asset = 'scr_family1', names = { 'scr_fam1_blood_headshot' }, scale = 1.0, dir = 'forward' },
        },
        kill = {
            { asset = 'core', names = { 'blood_chopper', 'blood_mist' }, scale = 0.6, dir = 'forward' },
        },
    },
    {
        key = 'gore', label = 'Cinematic gore',
        hit = {
            { asset = 'core', names = { 'blood_entry_sniper', 'blood_entry' }, scale = 1.2, dir = 'back' },
            { asset = 'core', names = { 'blood_exit' }, scale = 1.6, dir = 'forward', offset = 0.15 },
            { asset = 'core', names = { 'blood_mist' }, scale = 0.9, dir = 'forward', offset = 0.25 },
        },
        headshot = {
            { asset = 'scr_family1', names = { 'scr_fam1_blood_headshot' }, scale = 1.3, dir = 'forward' },
            { asset = 'core', names = { 'blood_entry_head_sniper' }, scale = 1.5, dir = 'forward' },
        },
        kill = {
            { asset = 'scr_solomon3', names = { 'scr_trev4_747_blood_impact' }, scale = 0.35, dir = 'up' },
        },
    },
    {
        key = 'pvp', label = 'PvP burst',
        hit = {
            { asset = 'core', names = { 'blood_entry' }, scale = 1.2, dir = 'back' },
            { asset = 'core', names = { 'bang_blood' }, scale = 1.0, dir = 'back' },
        },
        headshot = {
            { asset = 'core', names = { 'blood_headshot' }, scale = 1.4, dir = 'forward' },
            { asset = 'core', names = { 'blood_mist' }, scale = 0.8, dir = 'forward', offset = 0.15 },
        },
        kill = {
            { asset = 'scr_solomon3', names = { 'scr_trev4_747_blood_impact' }, scale = 0.3, dir = 'up' },
        },
    },
    {
        key = 'decals', label = 'Decals only',
        hit = {}, headshot = {}, kill = {},
    },

    -- The styles below copy what the uploaded bloodfx.dat packs do on hits, using effects from the
    -- base game. Run one of the [bloodfx-packs] resources to restyle these effects server-wide.
    {
        key = 'confetti', label = 'Confetti heads', -- light blue, slow solar system, pink tracers
        hit = { { asset = 'core', names = { 'blood_entry' }, scale = 1.0, dir = 'back' } },
        headshot = { { asset = 'core', names = { 'ent_dst_inflate_ball_clr' }, scale = 0.8, dir = 'back' } },
        headshotReplaces = true,
        kill = { { asset = 'core', names = { 'ent_dst_inflate_ball_clr' }, scale = 1.2, dir = 'up' } },
    },
    {
        key = 'white_confetti', label = 'White confetti', -- white confetti
        hit = { { asset = 'core', names = { 'blood_entry' }, scale = 1.0, dir = 'back' } },
        headshot = { { asset = 'core', names = { 'ent_dst_inflate_ball' }, scale = 0.8, dir = 'back' } },
        headshotReplaces = true,
        kill = { { asset = 'core', names = { 'ent_dst_inflate_ball' }, scale = 1.2, dir = 'up' } },
    },
    {
        key = 'marbles', label = 'Marble heads', -- custom marbles: no body particles, gumballs on the head
        hit = {},
        headshot = { { asset = 'core', names = { 'ent_dst_gen_gobstop' }, scale = 0.8, dir = 'back' } },
        headshotReplaces = true,
        kill = {},
    },
    {
        key = 'gumballs', label = 'Gumballs', -- slow confetti (new marbles)
        hit = { { asset = 'core', names = { 'ent_dst_gen_gobstop' }, scale = 0.7, dir = 'back' } },
        headshot = { { asset = 'core', names = { 'ent_dst_gen_gobstop' }, scale = 1.0, dir = 'back' } },
        headshotReplaces = true,
        kill = {},
    },
    {
        key = 'cash', label = 'Cash', -- uglyscrim
        hit = { { asset = 'core', names = { 'ent_brk_banknotes' }, scale = 0.8, dir = 'back' } },
        headshot = { { asset = 'core', names = { 'ent_brk_banknotes' }, scale = 1.1, dir = 'back' } },
        headshotReplaces = true,
        kill = {},
    },
    {
        key = 'water', label = 'Water', -- xans waterford
        hit = { { asset = 'core', names = { 'bul_water_heli' }, scale = 0.8, dir = 'back' } },
        headshot = { { asset = 'core', names = { 'bul_water_heli' }, scale = 1.1, dir = 'back' } },
        headshotReplaces = true,
        kill = {},
    },
    {
        key = 'oil', label = 'Oil', -- sleep (blood#2747)
        hit = { { asset = 'core', names = { 'liquid_splash_oil' }, scale = 0.8, dir = 'back' } },
        headshot = { { asset = 'core', names = { 'liquid_splash_oil' }, scale = 1.1, dir = 'back' } },
        headshotReplaces = true,
        kill = {},
    },
    {
        key = 'hatchet', label = 'Hatchet heads', -- w2mv blood x rem
        hit = { { asset = 'core', names = { 'blood_entry' }, scale = 1.0, dir = 'back' } },
        headshot = { { asset = 'core', names = { 'td_blood_hatchet' }, scale = 1.0, dir = 'forward' } },
        headshotReplaces = true,
        kill = {},
    },
    {
        key = 'chopper', label = 'Chopper heads', -- blood splat (gunz)
        hit = { { asset = 'core', names = { 'blood_entry' }, scale = 1.2, dir = 'back' } },
        headshot = { { asset = 'core', names = { 'blood_chopper' }, scale = 1.0, dir = 'forward' } },
        headshotReplaces = true,
        kill = {},
    },
    {
        key = 'stars', label = 'Star heads', -- red with stars
        hit = { { asset = 'core', names = { 'blood_entry' }, scale = 1.0, dir = 'back' } },
        headshot = { { asset = 'proj_indep_firework_v2', names = { 'scr_xmas_firework_burst_fizzle' }, scale = 0.6, dir = 'up' } },
        headshotReplaces = true,
        kill = {},
    },
}

-- Particle dictionaries the game's clown and alien blood need while those modes are on.
Config.NativeBloodAssets = {
    clown = 'scr_rcbarry2',
    alien = 'scr_rcbarry1',
}

-- Particles for damage that is not a bullet. Styles can override melee and blade.
Config.WeaponEffects = {
    melee = { { asset = 'core', names = { 'blood_melee_punch', 'blood_melee_blunt' }, scale = 1.0, dir = 'back' } },
    blade = { { asset = 'core', names = { 'blood_stab', 'td_blood_stab' }, scale = 1.0, dir = 'forward' } },
    vehicle = { { asset = 'core', names = { 'blood_wheel' }, scale = 1.0, dir = 'up' } },
    explosion = { { asset = 'core', names = { 'blood_mist' }, scale = 1.6, dir = 'up' } },
    gore = { { asset = 'core', names = { 'blood_chopper', 'blood_mist' }, scale = 1.0, dir = 'forward' } },
}

Config.Performance = {
    effectRange = 80.0,        -- hits further than this from your camera are ignored
    pedScanRange = 45.0,       -- range for death pools and bleeding
    maxDecals = 140,           -- effect decals alive at once; the oldest go first
    maxDecalsPerSecond = 30,
    maxParticlesPerSecond = 45,
    maxBleeders = 8,           -- nearest wounded peds that leave drops
    maxPools = 10,             -- pools spreading at the same time
}

Config.Pools = {
    decal = 1010,        -- 1010 = blood splatter. 9001 (solidPool_blood) gives a glossier liquid look.
    startDelay = 1500,   -- ms after death before the pool starts spreading
    startSize = 0.25,
}

Config.Bleeding = {
    fastest = 350,       -- ms between drops near death
    slowest = 2200,      -- ms between drops just under the threshold
    stopAfter = 90,      -- seconds after the last hit a ped stops dripping (0 = until healed)
}

Config.Splashback = {
    distance = 1.6,      -- metres; hits closer than this splash blood back on you
    packs = { 'Splashback_Face_0', 'Splashback_Face_1', 'Splashback_Torso_0', 'Splashback_Torso_1' },
}

-- Optional hook for "downed" systems that keep players alive while they look dead.
-- Return true to treat the ped as dead (gets a pool, stops bleeding). Example for qb-ambulancejob:
-- Config.IsPedDowned = function(ped) return IsEntityPlayingAnim(ped, 'dead', 'dead_a', 3) end
Config.IsPedDowned = nil

-- Test dummy spawned from the studio.
Config.Dummy = {
    model = 'a_m_y_hipster_01',
    health = 600,
}

-- Crime-scene painter (/bloodpaint).
Config.Painter = {
    range = 25.0,
    color = false,       -- force a colour for painted blood, e.g. '#3a0000'. false = the painter's own colour
    minSize = 0.1,
    maxSize = 4.0,
    kinds = {
        { key = 'splatter', label = 'Splatter', decal = 1010, size = 0.9 },
        { key = 'pool', label = 'Pool', decal = 1010, size = 1.8 },
        { key = 'spray', label = 'Spray', decal = 1015, size = 1.2, directional = true, aspect = 0.6 },
        { key = 'mist', label = 'Mist', decal = 1017, size = 1.4 },
        { key = 'drops', label = 'Drops', decal = 1110, size = 0.35 },
    },
}

-- Painted blood, synced to everyone.
Config.Scenes = {
    persist = true,          -- save to data/scenes.json and restore after restarts
    expireMinutes = 0,       -- 0 = painted blood stays until cleaned
    maxTotal = 600,          -- server-wide cap; the oldest is removed first
    streamDistance = 70.0,   -- clients draw painted blood within this distance
    maxVisible = 80,         -- painted decals a client draws at once (nearest first)
    maxPerSecond = 8,        -- placement rate limit per player
    maxPlaceDistance = 40.0, -- placements further than this from the painter are rejected (OneSync)
}

Config.Text = {
    noPermission = 'You are not allowed to do that.',
    painterOn = 'Crime-scene painter on.',
    painterOff = 'Crime-scene painter off.',
    cleaned = 'Cleaned blood within %dm.',
    cleanedSelf = 'You wiped the blood off yourself.',
    dummySpawned = 'Test dummy spawned. Shoot it.',
    dummyFailed = 'Could not load the dummy model.',
    nothingToUndo = 'Nothing to undo.',
}
