// The Last Bell - pixel art: palette, 8x8 tiles and 16x16 sprites, authored
// as ASCII art (one char per pixel, see kLegend in art.cpp) and decoded to
// palette indices at start-up. Index 0 is transparent in sprites.
#pragma once
#include <cstdint>

#include "pd/core.h"

namespace lb {

// 8x8 background tiles.
enum TileId {
    T_VOID = 0, T_GRASS, T_GRASS2, T_FLOWER, T_TALL, T_TREE, T_PINE, T_WATER,
    T_SAND, T_PATH, T_BRIDGE, T_MOUNTAIN, T_SNOW,
    T_WALL, T_ROOF, T_DOOR, T_WINDOW, T_SIGN, T_FENCE, T_WELL,
    T_DFLOOR, T_DWALL, T_PILLAR, T_STAIRUP, T_STAIRDN, T_BELL, T_CHEST,
    T_CARPET, T_COUNTER, T_BED, T_SHELF, T_CORRUPT,
    T_COUNT
};

// 16x16 sprites. Players: class*8 + dir*2 + frame (dir: 0 down, 1 up,
// 2 left, 3 right; frame 0/1 walk). Up sprites are derived from down (back of
// the head), right from left (mirrored), in art_init().
enum SpriteId {
    SPR_WARRIOR = 0, SPR_WHITE = 8, SPR_BLACK = 16, SPR_THIEF = 24,
    SPR_ELDER = 32, SPR_SHOP = 33, SPR_INN = 34, SPR_VILLAGER = 35, SPR_CHILD = 36,
    SPR_IMP = 40, SPR_WOLF, SPR_MOTH, SPR_NEWT, SPR_CINDER, SPR_BEETLE,
    SPR_BAT, SPR_WISP, SPR_TROLL, SPR_GOLEM,
    SPR_HOUND = 50, SPR_WURM, SPR_QGOLEM, SPR_REVENANT, SPR_VEY,
    SPR_COUNT
};

struct SpriteArt {
    uint8_t w, h;
    const uint8_t* px;   // w*h palette indices, 0 = transparent
};

// Fills g_palette (256 entries, for IndexedBlitter), g_tiles and the sprites.
void art_init();

extern pd::Color g_palette[256];
extern uint8_t g_tiles[T_COUNT][64];
const SpriteArt& sprite(int id);

// Palette indices reserved for the UI (240..254); art uses 0..26.
enum UiColor : uint8_t {
    C_WIN_BG = 240,   // window background (navy)
    C_WIN_FG = 241,   // window border and default text (white)
    C_YELLOW = 242,
    C_RED = 243,
    C_GREEN = 244,
    C_CYAN = 245,
    C_SEL = 246,      // selection row background (blue)
    C_DIM = 247,      // gray
    C_FLASH = 248,    // hit flash (white)
    C_SHADOW = 249,   // text shadow (dark)
    C_NO_BG = 255     // sentinel for text(): draw glyphs only
};

}  // namespace lb
