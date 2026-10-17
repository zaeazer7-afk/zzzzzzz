# BloodFX packs

Twelve bloodfx packs converted into FiveM resources. Each one streams its pack's particle file (`core.ypt`) to every player, so everyone sees that pack's tracers and effect styling without installing anything.

## Install

1. Copy the `[bloodfx-packs]` folder into your server's `resources` folder.
2. Add one line to `server.cfg` for the pack you want, for example:

   ```cfg
   ensure bfx_pink_tracers
   ```

3. Restart the server.

Run only one pack at a time, because they all replace the same game file. To switch packs, change the `ensure` line and restart.

## Packs

| Resource | Original pack | On hits, from its bloodfx.dat | Matching studio style |
| --- | --- | --- | --- |
| `bfx_lightblue_confetti` | 55$@Jodot | Head and neck hits burst into confetti, body hits bleed | Confetti heads |
| `bfx_slow_solarsystem` | Slow Solarsystem@Jodot | Head and neck hits burst into confetti, body hits bleed | Confetti heads |
| `bfx_pink_tracers` | pink tracers#Jodot | Head and neck hits burst into confetti, body hits bleed | Confetti heads |
| `bfx_white_confetti` | white confetti bfx | Head and neck hits burst into white confetti | White confetti |
| `bfx_custom_marbles` | custom marbles @Jodot | No body particles, gumballs on head hits | Marble heads |
| `bfx_slow_confetti` | new marbles (SlowConfetti @fearstory) | Every hit spills gumballs | Gumballs |
| `bfx_uglyscrim` | uglyscrim | Every hit throws banknotes | Cash |
| `bfx_xans_waterford` | xans waterford | Every hit splashes water | Water |
| `bfx_sleep` | blood#2747 | Every hit splashes oil | Oil |
| `bfx_w2mv_rem` | w2mv Blood x Rem | Head and neck hits use the hatchet takedown blood | Hatchet heads |
| `bfx_blood_splat` | blood splat @gunz | Heavier blood, rotor-blade blood on head hits | Chopper heads |
| `bfx_red_stars` | red with stars new | Body hits bleed, head hits burst into firework stars | Star heads |

Each resource keeps its pack's original `bloodfx.dat`, and its `fxmanifest.lua` credits the pack's author where the pack named one.

## What plays on hits

A pack's `bloodfx.dat` decides what plays when someone is shot. FiveM has no documented way to load that file from a server, so every pack's hit behaviour is also a style in BloodFX Studio. Players pick the matching style from the table to get the pack's hits.

The game's normal blood still shows under those styles. To replace it, players can set Game blood to Clown in the studio's Look tab.

Each `fxmanifest.lua` also has two commented-out lines that try to load `bloodfx.dat` for everyone through the data file type wired to the game's blood loader. That route is untested. Try it on a private server first, and comment the lines out again if players crash while joining.

## Before you go live

- **Check the pack loads.** Join, shoot something, and confirm the tracers and effects look like the pack.
- **Watch the server console.** FiveM may warn that `core.ypt` is an oversized asset. If it does, test with a few players before using the pack live.
- **If nothing changes,** FiveM didn't swap the file for your server. Players can still install the original RPF in their own FiveM mods folder.
