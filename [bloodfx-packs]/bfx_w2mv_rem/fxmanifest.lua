fx_version 'cerulean'
game 'gta5'

name 'bfx_w2mv_rem'
description 'w2mv blood x Rem: head and neck hits use the hatchet takedown blood'
author 'w2mv, Rem'
version '1.0.0'

--[[
    Converted from the "w2mv Blood x Rem" pack (TRacer.rpf).

    stream/core.ypt replaces GTA's core particle file for every player. That carries the
    pack's tracers and the look of its effects. Every pack replaces the same file, so run
    only one of them at a time.

    The pack's bloodfx.dat decides what plays on hits: head and neck hits use the hatchet takedown blood.
    Players get the same behaviour by picking the "Hatchet heads" style in BloodFX Studio.

    Untested: the two lines below try to load bloodfx.dat for everyone instead. Test them on
    a private server first, and comment them out again if players crash while joining.
]]
-- files { 'bloodfx.dat' }
-- data_file 'AMB_PROCEDURAL_BLOOD_FILE' 'bloodfx.dat'
