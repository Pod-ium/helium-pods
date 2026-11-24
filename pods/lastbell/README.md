# The Last Bell

A Final-Fantasy-style RPG for the **PicoCalc**, on the PicoPods libraries.
Turn-based random battles, a party of four wardens, skills learned on
level-up, shops, inns, four bell towers to climb and a Hollow King to unmake.
**Needs an RP2350** (Pico 2 / Pico 2 W).

## The story

Long ago the sky of **Vael** cracked and the world broke into a thousand
drifting isles, held aloft only by the song of the **Four Bells**. Now the
**Hollow King Vey** walks the dark and silences them one by one. You are four
Wardens of **Quayhaven**, the last free isle: ring the Bell of the Dawn, the
Tide, the Ember and the Gale, then climb the **Veilspire** and end the
silence.

## Installation

Flash `lastbell.uf2` in BOOTSEL mode, or copy it to `pico2-apps/` of the SD
card and start it from the UF2 Loader. Saves go to `/lastbell/save1.pqs` …
`save3.pqs` (a 64-byte header + the game state, 196 bytes); the game starts
fine without an SD card (saving then reports "No SD card!").

## Controls

| Key | Action |
|---|---|
| Arrows | Walk (hold to keep walking); move in menus |
| Enter / Space | Talk to NPCs, open chests, ring bells, confirm |
| Esc | Menu (Status, Items, Save, Help, Quit to title); cancel |
| F1 | Help |

In battle each living warden picks **Attack / Skill / Item / Run**; enemies
act after, fastest first. `Vanish` always escapes (except against Vey).

## How it works

- Everything is drawn into a **160x160 indexed framebuffer** (8x8 tiles,
  16x16 sprites, Terminus 6x12 text) that `pd::IndexedBlitter` scales 2x to
  the 320x320 panel by DMA, so only changed lines are sent (~330 KB of
  buffers, hence the RP2350 requirement). On the device the panel runs at
  50 MHz (`PD_PICOCALC_LCD_HZ`), as in PodCraft.
- Tiles, sprites and the palette are authored as ASCII art in
  `src/art.cpp` and decoded once at start-up; maps are ASCII grids in
  `src/maps.cpp` with NPCs, exits, chests, signs and boss triggers on top.
- Battles, dialogs, menus, shop and inn are blocking loops on the same
  framebuffer; input is the raw key layer (`poll_raw`), so held arrows walk
  and taps confirm.
- The RNG is a fixed-seed LCG: a new game replays exactly, which is what the
  scripted host test drives (`lastbell_script`: new game, quest, a meadow
  battle, a save; asserts the markers and the 196-byte save).

## Limits / simplifications

- One MP pool per warden (not per spell level), no equipment, no status
  ailments; enemies are physical with a few casters.
- The overworld is 64x64 with seven islands joined by sky-bridges; towers
  and the Veilspire are 24x24 single-purpose floors.

## Files

```
src/main.cpp     boot, title, map loop, dialogs, menus, shop/inn, triggers
src/art.*        palette, tiles, sprites (ASCII art)
src/maps.*       the 14 maps + NPCs, exits, chests, signs, bosses
src/data.*       classes, skills, items, enemies, encounter tables, story
src/game.*       party state (a POD, saved verbatim), levelling
src/battle.*     turn-based battles
src/render.*     framebuffer primitives + overworld screen, IndexedBlitter
src/storage.*    saves: /lastbell/saveN.pqs, 64-byte header + checksum
src/keys.*       raw key state
src/config.h     frame geometry, saves, keys, timing
```
