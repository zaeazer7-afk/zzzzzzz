fx_version 'cerulean'
game 'gta5'

name 'bfx_lightblue_confetti'
description 'Light blue with confetti: head and neck hits burst into confetti, body hits bleed'
author 'Jodot'
version '1.0.0'

--[[
    Converted from the "55$@Jodot" pack (light_blue_withfetti.rpf).

    stream/core.ypt replaces GTA's core particle file for every player. That carries the
    pack's tracers and the look of its effects. Every pack replaces the same file, so run
    only one of them at a time.

    The pack's bloodfx.dat decides what plays on hits: head and neck hits burst into confetti, body hits bleed.
    Players get the same behaviour by picking the "Confetti heads" style in BloodFX Studio.

    Untested: the two lines below try to load bloodfx.dat for everyone instead. Test them on
    a private server first, and comment them out again if players crash while joining.
]]
-- files { 'bloodfx.dat' }
-- data_file 'AMB_PROCEDURAL_BLOOD_FILE' 'bloodfx.dat'
