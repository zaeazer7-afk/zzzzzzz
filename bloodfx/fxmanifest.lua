fx_version 'cerulean'
game 'gta5'
lua54 'yes'

name 'bloodfx'
author 'bloodfx'
description 'Custom blood effects, an in-game editor (BloodFX Studio) and a synced crime-scene blood painter'
version '1.0.0'

shared_scripts {
    'config.lua',
    'shared/util.lua',
    'shared/schema.lua',
}

client_scripts {
    'client/settings.lua',
    'client/decals.lua',
    'client/fx.lua',
    'client/hits.lua',
    'client/peds.lua',
    'client/scenes.lua',
    'client/painter.lua',
    'client/studio.lua',
    'client/main.lua',
}

server_scripts {
    'server/sv_config.lua',
    'server/main.lua',
}

ui_page 'html/index.html'

files {
    'html/index.html',
    'html/style.css',
    'html/splat.js',
    'html/app.js',
}
