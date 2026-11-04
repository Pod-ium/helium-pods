// PodABC - drawing the big letters (and small ones for the menu).
#pragma once
#include <functional>

#include "assets.h"
#include "pd/core.h"

namespace abc {

// Draws glyph g scaled to `height` pixels (aspect kept; only scaling down is
// smooth, the full size is 1:1) with its top-left corner at (x, y). Each
// drawn row covers [band_x, band_x + band_w) of the screen, so a full-width
// band paints the background beside the glyph in the same pass (no flicker,
// no separate clear). `idle` runs every few rows (audio).
void draw_glyph(const Glyph& g, int x, int y, int height, pd::Color fg, pd::Color bg, int band_x,
                int band_w, const std::function<void()>& idle = nullptr);
// Width of glyph g drawn `height` pixels high.
int glyph_width(const Glyph& g, int height);

// The whole screen: background and `c` as large as it fits, centred.
void draw_big(char c, pd::Color fg, pd::Color bg, const std::function<void()>& idle = nullptr);

}  // namespace abc
