// The Last Bell - framebuffer primitives and the overworld screen.
#include "render.h"

#include <algorithm>
#include <cstdio>
#include <cstring>

#include "game.h"
#include "maps.h"
#include "pd/blit.h"
#include "pd/gfx.h"
#include "pd/pd.h"

namespace lb {

uint8_t fb[FB_W * FB_H];

namespace {
pd::IndexedBlitter g_blit;

inline void put(int x, int y, uint8_t c) {
    if ((unsigned)x < FB_W && (unsigned)y < FB_H) fb[y * FB_W + x] = c;
}

// Terminus glyph decode, same layout as pd::gfx (glyph_index in gfx.cpp).
int glyph_index(uint32_t cp) {
    if (cp >= 0x20 && cp < 0x7F) return int(cp - 0x20);
    if (cp >= 0xA0 && cp <= 0xFF) return int(cp - 0xA0 + 95);
    return '?' - 0x20;
}

void text_raw(int x, int y, const std::string& s, uint8_t fg, uint8_t bg, const pd::gfx::Font& f) {
    for (size_t i = 0; i < s.size(); ++i) {
        const uint32_t cp = uint8_t(s[i]);   // ASCII only in this game
        const uint8_t* glyph = f.data + glyph_index(cp) * f.h * f.bpr;
        for (int gy = 0; gy < f.h; ++gy) {
            const uint8_t* row = glyph + gy * f.bpr;
            for (int gx = 0; gx < f.w; ++gx) {
                const bool on = (row[gx >> 3] & (0x80 >> (gx & 7))) != 0;
                put(x + gx, y + gy, on ? fg : bg);
            }
        }
        x += f.w;
    }
}

}  // namespace

bool render_init() {
    if (!g_blit.begin(pd::gfx::display(), {FB_W, FB_H, FB_SCALE, 0, 0})) return false;
    g_blit.set_palette(g_palette, 256);
    return true;
}

void render_submit() {
    g_blit.poll();
    if (!g_blit.busy()) g_blit.submit(fb);
}

bool render_busy() { g_blit.poll(); return g_blit.busy(); }

void fb_clear(uint8_t c) { memset(fb, c, sizeof fb); }

void fb_fill(int x, int y, int w, int h, uint8_t c) {
    for (int j = y; j < y + h; ++j)
        for (int i = x; i < x + w; ++i) put(i, j, c);
}

void fb_frame(int x, int y, int w, int h, uint8_t c) {
    fb_fill(x, y, w, 1, c);
    fb_fill(x, y + h - 1, w, 1, c);
    fb_fill(x, y, 1, h, c);
    fb_fill(x + w - 1, y, 1, h, c);
}

void fb_tile(int x, int y, int t) {
    if (t < 0 || t >= T_COUNT) t = T_VOID;
    const uint8_t* px = g_tiles[t];
    for (int j = 0; j < TILE; ++j)
        for (int i = 0; i < TILE; ++i) put(x + i, y + j, px[j * TILE + i]);
}

void fb_sprite(int x, int y, int spr, uint8_t flash) {
    const SpriteArt& s = sprite(spr);
    for (int j = 0; j < s.h; ++j)
        for (int i = 0; i < s.w; ++i) {
            const uint8_t p = s.px[j * s.w + i];
            if (p == 0) continue;
            put(x + i, y + j, flash ? flash : p);
        }
}

void fb_sprite_scaled(int x, int y, int spr, int scale, uint8_t flash) {
    const SpriteArt& s = sprite(spr);
    for (int j = 0; j < s.h; ++j)
        for (int i = 0; i < s.w; ++i) {
            const uint8_t p = s.px[j * s.w + i];
            if (p == 0) continue;
            fb_fill(x + i * scale, y + j * scale, scale, scale, flash ? flash : p);
        }
}

void fb_text(int x, int y, const std::string& s, uint8_t fg, uint8_t bg) {
    if (bg != C_NO_BG) fb_fill(x, y, int(s.size()) * 6, 12, bg);
    text_raw(x, y, s, fg, bg == C_NO_BG ? 0 : bg, pd::gfx::font(pd::gfx::FONT_6x12));
}

void fb_text_big(int x, int y, const std::string& s, uint8_t fg, uint8_t bg) {
    const pd::gfx::Font& f = pd::gfx::font(pd::gfx::FONT_12x24);
    if (bg != C_NO_BG) fb_fill(x, y, int(s.size()) * f.w, f.h, bg);
    text_raw(x, y, s, fg, bg == C_NO_BG ? 0 : bg, f);
}

void fb_text_center(int y, const std::string& s, uint8_t fg, uint8_t bg) {
    fb_text((FB_W - fb_text_w(s)) / 2, y, s, fg, bg);
}

int fb_text_w(const std::string& s) { return int(s.size()) * 6; }
int fb_text_w_big(const std::string& s) { return int(s.size()) * 12; }

void win(int x, int y, int w, int h) {
    fb_fill(x, y, w, h, C_WIN_BG);
    fb_frame(x, y, w, h, C_WIN_FG);
}

std::vector<std::string> wrap_text(const std::string& s, int cells) {
    std::vector<std::string> out;
    std::string line;
    for (size_t i = 0; i < s.size(); ++i) {
        if (int(line.size()) >= cells && s[i] == ' ') { out.push_back(line); line.clear(); continue; }
        line += s[i];
        if (int(line.size()) >= cells) { out.push_back(line); line.clear(); }
    }
    if (!line.empty()) out.push_back(line);
    if (out.empty()) out.push_back("");
    return out;
}

// ---------------------------------------------------------------- overworld
void draw_map(const Game& g) {
    const MapDef& d = map_def(g.map);
    fb_clear(0);
    int camx = g.px * TILE + TILE / 2 - FB_W / 2;
    int camy = g.py * TILE + TILE / 2 - FB_H / 2;
    const int maxx = d.w * TILE - FB_W, maxy = d.h * TILE - FB_H;
    camx = maxx >= 0 ? std::clamp(camx, 0, maxx) : maxx / 2;   // small maps are centred
    camy = maxy >= 0 ? std::clamp(camy, 0, maxy) : maxy / 2;
    const int tx0 = camx / TILE, ty0 = camy / TILE;
    for (int j = 0; j < VIEW_TILES_Y; ++j)
        for (int i = 0; i < VIEW_TILES_X; ++i)
            fb_tile((tx0 + i) * TILE - camx, (ty0 + j) * TILE - camy, map_tile(g.map, tx0 + i, ty0 + j));
    // NPCs
    for (int i = 0; i < npc_count(); ++i) {
        const NpcDef& n = npc(i);
        if (n.map != g.map) continue;
        const int sx = n.x * TILE - camx, sy = n.y * TILE - camy;
        if (sx < -SPR || sy < -SPR || sx > FB_W || sy > FB_H) continue;
        fb_sprite(sx, sy, n.sprite);
    }
    // player
    const int px = g.px * TILE - camx, py = g.py * TILE - camy;
    fb_sprite(px, py, g.party[0].cls * 8 + g.dir * 2 + g.frame);
    // HUD: gold and bells, top right, with shadow
    char buf[32];
    snprintf(buf, sizeof buf, "%dG", g.gold);
    fb_text(FB_W - fb_text_w(buf) - 3, 3, buf, C_SHADOW);
    fb_text(FB_W - fb_text_w(buf) - 2, 2, buf, C_YELLOW);
    snprintf(buf, sizeof buf, "BELL %d/4", popcount_bells(g.flags));
    fb_text(FB_W - fb_text_w(buf) - 3, 15, buf, C_SHADOW);
    fb_text(FB_W - fb_text_w(buf) - 2, 14, buf, C_CYAN);
}

}  // namespace lb
