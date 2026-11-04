// PodABC - the embedded assets: voice clips and big letters
// (assets/*.bin, made by tools/make_assets.py, formats described there).
#pragma once
#include <cstdint>

namespace abc {

struct Clip {
    const uint8_t* data = nullptr;   // 8-bit mu-law, mono
    uint32_t length = 0;             // samples
};

struct Glyph {
    uint16_t w = 0, h = 0;
    const uint8_t* rle = nullptr;    // run-length coded 4-bit alpha
    uint32_t size = 0;
};

// Sample rate of the clips (16 kHz).
uint32_t clip_rate();
// A clip by name: "a".."z", "0".."9", "riprova", "jingle"... False if missing.
bool clip(const char* name, Clip& out);
// The glyph for 'A'..'Z', '0'..'9' or '?' (lower-case letters give the capital).
bool glyph(char c, Glyph& out);

// Checks both packs (magic, version, offsets inside the data).
bool assets_ok();

}  // namespace abc
