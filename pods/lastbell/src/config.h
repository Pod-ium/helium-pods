// The Last Bell - an RPG for the PicoCalc, on the PicoPods libraries.
// Configuration: frame geometry, tiles, saves, keys.
#pragma once

// ---------------------------------------------------------------- rendering
// The game renders into a 160x160 indexed framebuffer, scaled 2x to the
// 320x320 panel by pd::IndexedBlitter (DMA): 8x8 tiles give a 20x20-tile
// viewport, 16x16 sprites. Text is the Terminus 6x12 font drawn into the
// framebuffer (12x24 on the panel), the title uses 12x24 (24x48).
#define FB_W        160
#define FB_H        160
#define FB_SCALE    2
#define TILE        8           // tile size in framebuffer pixels
#define VIEW_TILES_X (FB_W / TILE)   // 20
#define VIEW_TILES_Y (FB_H / TILE)   // 20
#define SPR         16          // character sprite size

// ---------------------------------------------------------------- gameplay
#define PARTY_SIZE  4
#define MAX_LEVEL   15
#define ENCOUNTER_RATE 7        // 1 in N steps on dangerous ground

// ---------------------------------------------------------------- saves
#define SAVE_DIR    "/lastbell"
#define SAVE_SLOTS  3
#define SAVE_MAGIC  "LBEL"
#define SAVE_VERSION 1

// ---------------------------------------------------------------- keys
// Arrows move and drive the menus, Enter/Space confirm and talk, Esc opens
// the menu (overworld) or cancels, F1 shows the controls.
#define KEY_CONFIRM1 0x0A       // Enter
#define KEY_CONFIRM2 0x20       // Space

// ---------------------------------------------------------------- timing
#define MOVE_REPEAT_MS 150      // held arrow: one step per this
#define MOVE_DELAY_MS  230      // before the repeat starts
#define TYPE_MS       18        // dialog typewriter, ms per character
