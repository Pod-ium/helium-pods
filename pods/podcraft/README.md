# PodCraft

A small voxel sandbox game for the **ClockworkPi PicoCalc** with a **Raspberry Pi Pico 2 W** (RP2350).
A procedurally generated block world, real-time 3D rendering with a voxel ray caster running
on both cores, block building and breaking, flying, see-through water and saves on the SD card.

PodCraft is a pod of the PicoPods family: display, keyboard, SD card, crash screen and the
PC/browser builds come from the shared libraries ([hydrogenium-pods](https://github.com/Pod-ium/hydrogenium-pods));
this folder holds the game engine and the main loop.

## Installation

1. Copy `podcraft.uf2` to the Pico 2 W in BOOTSEL mode, or put it in the `pico2-apps/` folder of the SD card and start it from **UF2 Loader**.
2. The SD card is only used for saves: `/podcraft/world1.pcw` … `world3.pcw` (FAT32 or exFAT, ~256 KB each). The game also starts without a card.

## Controls

| Key | Action |
|---|---|
| W A S D | move |
| Arrows | look around |
| Space | jump / swim / fly up |
| C | fly down |
| J, Del or Backspace | break the targeted block |
| K or Enter | place the selected block |
| 1-9, Q / E | pick a hotbar slot |
| Tab | change the block type in the slot |
| F | flying on/off |
| F1 | controls screen |
| F3 | debug info (FPS, cores, coordinates, seed) |
| Esc | menu (save, load, new world, view distance, flying) |

On the title screen: Enter to play, N for a new world, L to load slot 1.

## Building

PodCraft is one of the pods of this repository. From the repository root:

```sh
cmake -B build -DPICO_SDK_PATH=/path/to/pico-sdk      # PicoCalc + Pico 2 W
cmake --build build -j --target podcraft
# -> build/pods/podcraft/podcraft.uf2
```

It needs an RP2350 (Pico 2 / Pico 2 W): the world takes 256 KB of RAM and the two frames another 100 KB, more than an original Pico has. For the Pico 2 without Wi-Fi: `-DPD_BOARD=pico2`.

On the PC (`cmake -B build-host -DPD_TERMINAL=host`) the game opens in a window (with SDL2), or runs with scripted keys and saves screenshots:

```sh
PD_SDCARD=./sd PD_FRAMES=./frames PD_KEYS="wait:2500,shot,enter,wait:500,f3,wait:500,shot" ./build-host/pods/podcraft/podcraft
```

## How it works

- **Rendering:** per-pixel DDA ray casting at 160×160, scaled 2× on the 320×320 display. Fog, procedural 16×16 textures, per-face shading, see-through blocks (leaves, glass) and semi-transparent water. The ray caster runs from RAM.
- **Two cores:** the rows of each frame are handed out one at a time by a shared counter. Core 1 only renders; core 0, while core 1 works, sends the previous frame to the display and reads the keyboard (slow I2C reads, ~16 ms each), then takes the remaining rows. In the browser there is a single thread and core 0 does everything.
- **Display (`src/screen.cpp`):** the panel only takes 18-bit colour over SPI, so a 2× frame is 300 KB, which does not fit in RAM next to the world. The frame goes out in bands of 8 rows (16 rows on the panel, 15 KB): one band is converted while the previous one is in flight by DMA. Display SPI at 50 MHz (49 ms per frame), as PodBoy.
- **World:** 128×128×32 blocks, toroidal (the edges wrap around), 4 bits per block. Terrain from periodic noise, snowy mountains, beaches, lakes, trees, caves and gold veins.
- **Clock:** 200 MHz (voltage set by the library).
- **Memory:** ~443 KB of static RAM out of 520 KB.

## Troubleshooting

| Symptom | What to try |
|---|---|
| Display glitches | Set `PD_PICOCALC_LCD_HZ` back to `25000000` in `CMakeLists.txt` (slower: 98 ms per frame) |
| Inverted colours | `PD_PICOCALC_LCD_INVERT=0` in `CMakeLists.txt` |
| Instability or random crashes | `SYS_CLOCK_KHZ 150000` in `src/config.h` (no overclock) |
| Sluggish keyboard | `PD_PICOCALC_KBD_GAP_US=2000` in `CMakeLists.txt` |
| Slow game | Lower view distance from the menu, or smaller `RW`/`RH` in `src/config.h` (e.g. 128×128: the image stays centred, with a black border) |

On a crash a red screen shows the program counter. Serial output (UART0, 115200 baud) is enabled for debugging.

## Layout

```
src/main.cpp      game loop, physics, HUD, menus, title, work split between the cores
src/render.cpp    voxel ray caster (in RAM)
src/world.cpp     world generation, block picking
src/textures.cpp  procedural textures
src/gfx.cpp       3x5 font and 2D primitives on the 160x160 frame
src/screen.cpp    display output: 2x scaling, 18-bit, in bands by DMA
src/storage.cpp   saves (pd_fs)
src/config.h      resolution, world, view distance, clock
```

## Licences

- PodCraft code: yours.
- FatFs (ChaN): BSD-style licence, see `lib/services/fs/fatfs/LICENSE.txt`.
- Terminus font (title screen, Dimitar Toshkov Zhekov): SIL Open Font License 1.1.
