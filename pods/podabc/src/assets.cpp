// PodABC - reading the embedded asset packs.
#include "assets.h"

#include <cstring>

extern const uint8_t abc_voice[];
extern const uint32_t abc_voice_size;
extern const uint8_t abc_glyphs[];
extern const uint32_t abc_glyphs_size;

namespace abc {

namespace {

uint16_t u16(const uint8_t* p) { return uint16_t(p[0] | (p[1] << 8)); }
uint32_t u32(const uint8_t* p) { return p[0] | (p[1] << 8) | (p[2] << 16) | (uint32_t(p[3]) << 24); }

constexpr uint32_t kVoiceHeader = 12, kVoiceEntry = 20;
constexpr uint32_t kGlyphHeader = 8, kGlyphEntry = 16;

uint16_t count(const uint8_t* pack, uint32_t size, const char* magic, uint32_t count_at) {
    if (size < 8 || memcmp(pack, magic, 4) != 0 || u16(pack + 4) != 1) return 0;
    return u16(pack + count_at);
}

uint16_t voice_count() { return count(abc_voice, abc_voice_size, "PABV", 8); }
uint16_t glyph_count() { return count(abc_glyphs, abc_glyphs_size, "PABG", 6); }

}  // namespace

uint32_t clip_rate() { return voice_count() ? u16(abc_voice + 6) : 16000; }

bool clip(const char* name, Clip& out) {
    const uint16_t n = voice_count();
    for (uint16_t i = 0; i < n; ++i) {
        const uint8_t* e = abc_voice + kVoiceHeader + i * kVoiceEntry;
        if (strncmp(reinterpret_cast<const char*>(e), name, 12) != 0) continue;
        const uint32_t off = u32(e + 12), len = u32(e + 16);
        if (off > abc_voice_size || len > abc_voice_size - off) return false;
        out.data = abc_voice + off;
        out.length = len;
        return true;
    }
    return false;
}

bool glyph(char c, Glyph& out) {
    if (c >= 'a' && c <= 'z') c = char(c - 32);
    const uint16_t n = glyph_count();
    for (uint16_t i = 0; i < n; ++i) {
        const uint8_t* e = abc_glyphs + kGlyphHeader + i * kGlyphEntry;
        if (e[0] != uint8_t(c)) continue;
        const uint32_t off = u32(e + 8), size = u32(e + 12);
        if (off > abc_glyphs_size || size > abc_glyphs_size - off) return false;
        out.w = u16(e + 2);
        out.h = u16(e + 4);
        out.rle = abc_glyphs + off;
        out.size = size;
        return out.w && out.h;
    }
    return false;
}

bool assets_ok() {
    const uint16_t nv = voice_count(), ng = glyph_count();
    if (!nv || !ng) return false;
    if (kVoiceHeader + nv * kVoiceEntry > abc_voice_size) return false;
    if (kGlyphHeader + ng * kGlyphEntry > abc_glyphs_size) return false;
    Clip c;
    Glyph g;
    return clip("riprova", c) && clip("jingle", c) && glyph('A', g) && glyph('?', g);
}

}  // namespace abc
