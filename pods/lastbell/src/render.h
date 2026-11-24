// The Last Bell - rendering: everything is drawn into a 160x160 indexed
// framebuffer (fb) that pd::IndexedBlitter scales 2x to the 320x320 panel by
// DMA, so only changed lines are converted and sent.
#pragma once
#include <cstdint>
#include <string>
#include <vector>

#include "art.h"
#include "config.h"

namespace lb {

struct Game;

extern uint8_t fb[FB_W * FB_H];

bool render_init();        // begins the blitter; false if the buffers do not fit
void render_submit();      // sends fb if changed and the panel is free
bool render_busy();

// ---- primitives into fb
void fb_clear(uint8_t c);
void fb_fill(int x, int y, int w, int h, uint8_t c);
void fb_frame(int x, int y, int w, int h, uint8_t c);
void fb_tile(int x, int y, int t);
void fb_sprite(int x, int y, int spr, uint8_t flash = 0);          // flash: draw all pixels as `flash`
void fb_sprite_scaled(int x, int y, int spr, int scale, uint8_t flash = 0);
void fb_text(int x, int y, const std::string& s, uint8_t fg, uint8_t bg = C_NO_BG);      // 6x12
void fb_text_big(int x, int y, const std::string& s, uint8_t fg, uint8_t bg = C_NO_BG);  // 12x24
void fb_text_center(int y, const std::string& s, uint8_t fg, uint8_t bg = C_NO_BG);
int fb_text_w(const std::string& s);
int fb_text_w_big(const std::string& s);
void win(int x, int y, int w, int h);   // RPG window: navy fill, white frame
std::vector<std::string> wrap_text(const std::string& s, int cells);

// ---- screens
void draw_map(const Game& g);           // map, NPCs, player, HUD (gold, bells)

}  // namespace lb
