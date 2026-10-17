fx_version 'cerulean'
game 'gta5'

name 'bfx_red_stars'
description 'Red with stars: body hits bleed and head hits burst into firework stars'
author 'unknown'
version '1.0.0'

--[[
    Converted from the "red with stars new" pack (lightblue.rpf).

    stream/core.ypt replaces GTA's core particle file for every player. That carries the
    pack's tracers and the look of its effects. Every pack replaces the same file, so run
    only one of them at a time.

    The pack's bloodfx.dat decides what plays on hits: body hits bleed and head hits burst into firework stars.
    Players get the same behaviour by picking the "Star heads" style in BloodFX Studio.

    Untested: the two lines below try to load bloodfx.dat for everyone instead. Test them on
    a private server first, and comment them out again if players crash while joining.
]]
-- files { 'bloodfx.dat' }
-- data_file 'AMB_PROCEDURAL_BLOOD_FILE' 'bloodfx.dat'
