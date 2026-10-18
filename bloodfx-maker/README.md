# BloodFX Maker

Makes custom bullet tracers, hit effects and muzzle flashes for FiveM, starting from the tracer packs you already have. Saving always gives you one zip with both files: a new tracer `.rpf` for your FiveM `mods` folder and a new `bloodfx.dat`.

Open `index.html` in a browser, or use the hosted copy in Claude. Your files are read and rebuilt in the page itself and are never uploaded. The page loads pako (for compression) from cdnjs and its fonts from Google Fonts, so it needs an internet connection.

## Pages

| Page | What it does |
| --- | --- |
| Home | Open a pack, quick starts, the open pack's facts. |
| Tracer | 30 templates, a live preview, the tracer sheet before and after, and the beam, smoke, timing and glow controls. |
| Blood | 24 presets, a hit map (head, body, animals × guns, shotguns, snipers, melee, explosions), and an effect browser over every effect in the pack. |
| Muzzle flash | Recolours the pack's muzzle flash particles. |
| Pack explorer | Every effect (with loop and duration), texture and file in the pack's `core.ypt`, plus the tracer's own numbers. |
| Install | What gets saved and where each file goes. |
| Settings | Accent colour, preview defaults, tips, start over. |

Undo and redo cover every change (Ctrl+Z, Ctrl+Y). Drop a `.rpf`, `core.ypt`, `bloodfx.dat` or a GTA screenshot anywhere on the window to open it; a screenshot becomes the preview's background.

## Tracer

Open a tracer pack (`.rpf`, such as `bluetrace.rpf`) or a loose `core.ypt`. The maker finds `core.ypt` inside the pack, rewrites the gun tracer (`bullet_tracer`), and saves the pack again with the same folders and file names. Everything else in the pack is copied unchanged.

| Control | What it changes in `core.ypt` |
| --- | --- |
| Beam look | The beam frame of the tracer sheet `ptfx_bullet_tracer` (frame 4 of 4). Pack's keeps it; Recolour keeps its exact pixels and paints them in your colour; None, Solid, Neon and Twin draw a new beam. Drawn beams can fade to a second colour along the tracer. |
| Smoke look | Frames 1–3 of the same sheet, which the game steps through over the smoke's life. Pack's and Recolour work like the beam's; Straight draws a crisp line with a ragged edge, Wavy a wavy line. |
| Tint | With the pack's beam and smoke both kept, multiplies their colour. |
| Glow | Emissive intensity of the `bullet_tracer` particle (stock 15). |
| Thickness | Zoom scalar of the tracer emitter (stock 1). |
| Size far away | Size of the tracer at long range, from the emitter's LOD evolution (stock 200). |
| Speed | Playback rate of the whole effect. The emitter's own rate is reset to 1 so the slider is the only speed. |
| Stays for | Effect duration (stock 0.1 s). |
| Beam life | Particle life of the tracer emitter (stock 0.05–0.07 s). |

Controls start at the pack's own values, and anything you don't touch is written back unchanged.

Recolouring only changes the two colour endpoints of each DXT1 or DXT5 block, so the shape and transparency stay exactly as they were at every mip level. BC7 sheets (in the sleep, slow confetti, white confetti and xans packs) can't be recoloured; keep them whole or draw a new beam and smoke, which is written as DXT5 in the same space.

## Blood

`bloodfx.dat` picks an effect for each body part and weapon type. Start from the built-in file (the layout all of your packs share, with normal blood everywhere) or open your own, then pick hits on the map and an effect for them.

Rows in `bloodfx.dat` are shared between hits. When a change only covers some of the hits that use a row, the maker copies that row to a new id and points just those hits at the copy, so nothing else changes. Splat colour writes the decal tint (the game uses 50 0 0), and splatter size scales the decal sizes.

The effect browser lists every effect in the open pack (or in GTA's standard `core.ypt`), sorted into categories, and warns when an effect loops, runs long, or isn't in the pack.

## Muzzle flash

Recolours every particle whose name contains `muz` (or other text you type), with a tick box for each. Each keyframe keeps its own brightness and alpha, so a flash that fades still fades. It can also set their glow.

## Install

1. Close FiveM.
2. Put the `.rpf` from the zip's `mods` folder in `%localappdata%\FiveM\FiveM.app\mods`. Remove the old copy of that pack, and keep only one pack that replaces `core.ypt`.
3. Put `bloodfx.dat` in `%localappdata%\FiveM\FiveM.app\citizen\common\data\effects`. Make the folders if they're missing, and back up any `bloodfx.dat` already there.
4. Start FiveM. Servers with strict client-mod rules ignore these files.

## Limits

- Not tested in game. Every output was checked by opening it again and by exporting the rebuilt `core.ypt` with CodeWalker, for all 12 reference packs.
- The preview is a sketch drawn from the pack's real tracer sheet, smoke life and fade, not GTA's renderer. CodeWalker and OpenIV can't play particle effects either.
- Encrypted RPFs aren't supported. Open the pack in OpenIV, switch it to unencrypted (OpenIV) and save it first.
- Only the standard gun tracer (`bullet_tracer`) is retimed and recoloured. Shotgun, machine-gun and jet tracers draw from the same sheet, so they pick up its new look mixed with their own colour and timing.

## Development

```sh
node src/build.js          # writes index.html from src/
node test/test_dat.js      # bloodfx.dat editing, on every bloodfx.dat in [bloodfx-packs]
node test/test_tracer.js   # tracer patching on every core.ypt in [bloodfx-packs], plus a nested .rpf round trip
```

`src/core/bfxcore.js` holds the RPF, resource, `core.ypt` and `bloodfx.dat` code and runs in browsers and Node. `src/ui/` is the interface: `index.html` and `app.css` are the frame, `app.js` holds the state, undo, loading and saving, and `pages/` has one file per page. `preview.js` draws the preview, `art.js` the thumbnails, and `platform.js` hides the difference between a browser, Claude and the Windows app. The build inlines everything into one page, with `src/base_bloodfx.dat`, `src/core_effects.json` (the effect names in GTA's standard `core.ypt`) and `src/long_effects.json` (the standard effects that loop or run longer than 4 s).
