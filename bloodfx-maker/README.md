# BloodFX Maker

A one-page tool that makes custom bullet tracers and hit effects for FiveM, starting from the packs you already have. Save always gives you one zip with both files: a new tracer `.rpf` for your FiveM `mods` folder and a new `bloodfx.dat`. The tracer is built from one of your packs, so Save asks for a pack if none is loaded.

Open `index.html` in a browser, or use the hosted copy in Claude. Your files are read and rebuilt in the page itself and are never uploaded. The page loads one script (pako, for compression) from cdnjs, so it needs an internet connection.

## Tracer

Load a tracer pack (`.rpf`, such as `bluetrace.rpf` or `TRacer.rpf`) or a loose `core.ypt`. The maker finds `core.ypt` inside the pack, rewrites the gun tracer (`bullet_tracer`), and saves the pack again with the same folders and file names. Everything else in the pack is copied unchanged.

| Control | What it changes in `core.ypt` |
| --- | --- |
| Look, colour, width | Redraws the tracer sheet `ptfx_bullet_tracer`. Frame 3 of its four frames is the beam: none (smoke only), solid, neon (thin white core) or twin lines. The beam particle colour and emitter tint are set to white so the drawn colour shows as picked. |
| Smoke trail | Frames 0–2 of the same sheet. The game plays them in order over the smoke's life. Straight draws soft smoke that widens, fades and breaks up from frame to frame; wavy draws a hard wavy line. |
| Keep pack's | Leaves the sheet alone. Tint multiplies the pack's own colours instead. |
| Glow | Emissive intensity of the `bullet_tracer` particle (stock 15–16). |
| Thickness | Zoom scalar of the tracer emitter (stock 1). |
| Far-away size | Size of the tracer at long range, from the emitter's LOD evolution (stock 200). |
| Speed | Playback rate of the whole effect. Lower is a slower beam. The emitter's own rate is reset to 1 so the slider is the only speed. |
| Stays for | Effect duration (stock 0.1 s). |
| Beam life | Particle life of the tracer emitter (stock 0.05–0.07 s). |

Controls start at the loaded pack's values, and anything you don't touch is written back unchanged. DXT1 sheets are redrawn as DXT1; DXT5 and BC7 sheets are written as DXT5, which takes the same space.

## Blood

`bloodfx.dat` picks an effect for each body part and weapon type. Start from the built-in file (the layout all of your packs share, with normal blood everywhere) or load your own, then add rules such as "when the head is hit by guns, shotguns and snipers, play Cash".

Rows in `bloodfx.dat` are shared between hits. When a rule only covers some of the hits that use a row, the maker copies that row to a new id and points just those hits at the copy, so nothing else changes. For example, in the stock layout explosions use the head row, so a headshot rule no longer turns explosions into confetti too. Splat colour writes the decal tint (the game uses 50 0 0), and splatter size scales the decal sizes.

The effect picker lists effects from your packs and from GTA's standard `core.ypt`. Any other name works too; the maker warns when it isn't in the loaded pack or in the standard file.

## Install

1. Close FiveM.
2. Put the `.rpf` from the zip's `mods` folder in `%localappdata%\FiveM\FiveM.app\mods`. Remove the old copy of that pack, and keep only one pack that replaces `core.ypt`.
3. Put `bloodfx.dat` in `%localappdata%\FiveM\FiveM.app\citizen\common\data\effects`. Make the folders if they're missing, and back up any `bloodfx.dat` already there.
4. Start FiveM. Servers with strict client-mod rules ignore these files.

## Limits

- Not tested in game. Every output was checked by opening it again and by exporting the rebuilt `core.ypt` with CodeWalker, for all 12 reference packs. The preview in the page is a sketch of the look, not a render of GTA's lighting.
- Encrypted RPFs aren't supported. Open the pack in OpenIV, switch it to unencrypted (OpenIV) and save it first.
- Only the standard gun tracer (`bullet_tracer`) is retimed and recoloured. Shotgun, machine-gun and jet tracers draw from the same sheet, so they pick up its new shape mixed with their own colour and timing.

## Development

```sh
node src/build.js          # writes index.html from src/
node test/test_dat.js      # bloodfx.dat editing, on every bloodfx.dat in [bloodfx-packs]
node test/test_tracer.js   # tracer patching on every core.ypt in [bloodfx-packs], plus a nested .rpf round trip
```

`src/core/bfxcore.js` holds the RPF, resource, `core.ypt` and `bloodfx.dat` code and runs in browsers and Node. `src/ui/legacy-page.html` is the page. The build inlines the core, `src/base_bloodfx.dat` and `src/core_effects.json` (the effect names in GTA's standard `core.ypt`).
