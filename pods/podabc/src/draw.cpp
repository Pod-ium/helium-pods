// PodABC - glyph drawing: the run-length data is decoded one source row at a
// time and box-filtered down to the target size, one screen row at a time.
#include "draw.h"

#include <algorithm>
#include <vector>

#include "pd/gfx.h"

namespace abc {

int glyph_width(const Glyph& g, int height) {
    if (!g.h) return 0;
    return std::max(1, (g.w * height + g.h / 2) / g.h);
}

void draw_glyph(const Glyph& g, int x, int y, int height, pd::Color fg, pd::Color bg, int band_x,
                int band_w, const std::function<void()>& idle) {
    if (!g.rle || height <= 0 || band_w <= 0) return;
    height = std::min<int>(height, g.h);   // never scaled up
    const int w = glyph_width(g, height);
    pd::Color shade[16];
    for (int i = 0; i < 16; ++i) shade[i] = pd::blend(bg, fg, uint8_t(i * 17));

    std::vector<uint32_t> sum(w);     // alpha summed per target column
    std::vector<uint16_t> cnt(w);     // source columns per target column
    for (int sx = 0; sx < g.w; ++sx) cnt[sx * w / g.w]++;
    std::vector<pd::Color> row(band_w, bg);

    const uint8_t* p = g.rle;
    const uint8_t* end = g.rle + g.size;
    int run = 0, val = 0;
    int ty = 0, rows_in = 0;
    auto emit = [&](int ty) {
        std::fill(row.begin(), row.end(), bg);
        for (int tx = 0; tx < w; ++tx) {
            const int bx = x + tx - band_x;
            if (bx < 0 || bx >= band_w) continue;
            const int div = cnt[tx] * rows_in;
            row[bx] = shade[div ? (sum[tx] + div / 2) / div : 0];
        }
        const int sy = y + ty;
        if (sy >= 0 && sy < pd::gfx::height()) pd::gfx::blit_rgb(band_x, sy, band_w, 1, row.data());
        std::fill(sum.begin(), sum.end(), 0);
        rows_in = 0;
        if (idle && (ty & 7) == 7) idle();
    };
    for (int sy = 0; sy < g.h; ++sy) {
        const int t = sy * height / g.h;
        if (t != ty) {
            emit(ty);
            ty = t;
        }
        for (int sx = 0; sx < g.w; ++sx) {
            if (!run) {
                if (p >= end) return;   // truncated data
                const uint8_t b = *p++;
                val = b >> 4;
                run = b & 15;
                if (!run) run = 16 + (p < end ? *p++ : 0);
            }
            --run;
            sum[sx * w / g.w] += uint32_t(val);
        }
        ++rows_in;
    }
    emit(ty);
}

void draw_big(char c, pd::Color fg, pd::Color bg, const std::function<void()>& idle) {
    const int sw = pd::gfx::width(), sh = pd::gfx::height();
    Glyph g;
    if (!glyph(c, g)) {
        pd::gfx::clear(bg);
        return;
    }
    // As large as the screen allows (the glyphs are made for 320x320).
    int h = std::min<int>(g.h, sh - 16);
    if (glyph_width(g, h) > sw - 16) h = h * (sw - 16) / glyph_width(g, h);
    const int w = glyph_width(g, h);
    const int x = (sw - w) / 2, y = (sh - h) / 2;
    if (y > 0) pd::gfx::fill(0, 0, sw, y, bg);
    draw_glyph(g, x, y, h, fg, bg, 0, sw, idle);
    const int bottom = y + h;
    if (bottom < sh) pd::gfx::fill(0, bottom, sw, sh - bottom, bg);
}

}  // namespace abc
