// PodABC: the embedded assets, the clip player (sample by sample, through a
// capturing audio sink) and the big-letter drawing, on the host terminal.
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <vector>

#include "assets.h"
#include "draw.h"
#include "pd/pd.h"
#include "pd/terminals/host.h"
#include "sound.h"

namespace {

int failures = 0;
#define CHECK(cond)                                                        \
    do {                                                                   \
        if (!(cond)) {                                                     \
            fprintf(stderr, "%s:%d: CHECK failed: %s\n", __FILE__, __LINE__, #cond); \
            ++failures;                                                    \
        }                                                                  \
    } while (0)

struct Capture : pd::host::AudioSink {
    uint32_t rate = 0;
    std::vector<int16_t> frames;   // left channel
    bool hold = false;             // true: nothing plays (the queue fills up)
    int pending = 0;
    bool open(uint32_t r) override {
        rate = r;
        return true;
    }
    void close() override {}
    void queue(const int16_t* stereo, int n) override {
        for (int i = 0; i < n; ++i) {
            CHECK(stereo[2 * i] == stereo[2 * i + 1]);   // mono on both sides
            frames.push_back(stereo[2 * i]);
        }
        pending += n;
    }
    int queued() override { return hold ? pending : 0; }   // otherwise plays instantly
    void clear() override { pending = 0; }
};

int16_t ulaw(uint8_t b) {   // reference G.711 decoder
    b = uint8_t(~b);
    int t = ((b & 0x0F) << 3) + 0x84;
    t <<= (b & 0x70) >> 4;
    return int16_t((b & 0x80) ? (0x84 - t) : (t - 0x84));
}

void test_assets() {
    CHECK(abc::assets_ok());
    CHECK(abc::clip_rate() == 16000);
    const char* phrases[] = {"riprova", "ottimo", "bene", "evviva", "esatto",
                             "trova",   "ascolta", "premi", "ciao", "jingle"};
    for (const char* p : phrases) {
        abc::Clip c;
        CHECK(abc::clip(p, c) && c.length > 1600);   // > 0.1 s
    }
    for (char ch : std::string("abcdefghijklmnopqrstuvwxyz0123456789")) {
        const char name[2] = {ch, 0};
        abc::Clip c;
        abc::Glyph g;
        CHECK(abc::clip(name, c) && c.length > 1600 && c.length < 32000);
        CHECK(abc::glyph(ch, g) && g.w <= 304 && g.h <= 304 && (g.w == 304 || g.h == 304));
    }
    abc::Clip c;
    abc::Glyph g;
    CHECK(!abc::clip("nessuno", c));
    CHECK(!abc::glyph('#', g));
}

void test_sound(Capture& cap) {
    abc::Sound snd;
    snd.init(pd::terminal().audio(), 100);
    CHECK(cap.rate == 32000);
    abc::Clip a, b;
    CHECK(abc::clip("a", a) && abc::clip("bene", b));
    snd.play("a");
    snd.then("bene", 100);
    for (int guard = 0; snd.busy() && guard < 100000; ++guard) snd.pump();
    CHECK(!snd.busy());
    const size_t gap = 100 * 32;   // 100 ms at 32 kHz
    CHECK(cap.frames.size() == 2 * a.length + gap + 2 * b.length);
    if (cap.frames.size() != 2 * a.length + gap + 2 * b.length) return;
    // 2x: every second frame is the sample, the ones between are midpoints.
    int16_t prev = 0;
    for (uint32_t i = 0; i < a.length; ++i) {
        const int16_t s = ulaw(a.data[i]);
        CHECK(cap.frames[2 * i + 1] == s);
        CHECK(cap.frames[2 * i] == int16_t((prev + s) / 2));
        prev = s;
        if (failures > 10) return;
    }
    for (size_t i = 0; i < gap; ++i) CHECK(cap.frames[2 * a.length + i] == 0);
    CHECK(cap.frames[2 * a.length + gap + 1] == ulaw(b.data[0]));
    // Loud enough to be heard: RMS of "a" over ~ -25 dBFS.
    double e = 0;
    for (uint32_t i = 0; i < 2 * a.length; ++i) e += double(cap.frames[i]) * cap.frames[i];
    CHECK(std::sqrt(e / (2 * a.length)) > 1800);

    // The queue is kept ~40 ms ahead, and play() drops it and starts over.
    cap.frames.clear();
    cap.hold = true;
    snd.play("ciao");
    snd.pump();
    CHECK(cap.frames.size() >= 1000 && cap.frames.size() <= 1280);
    const size_t before = cap.frames.size();
    snd.play("a");
    CHECK(cap.pending > 0 && cap.pending <= 1280);
    cap.hold = false;
    for (int guard = 0; snd.busy() && guard < 100000; ++guard) snd.pump();
    CHECK(cap.frames.size() == before + 2 * a.length);
}

void test_draw() {
    const pd::Color fg = 0xE53935, bg = 0xFFFBF0;
    abc::draw_big('I', fg, bg);
    CHECK(pd::host::pixel(160, 160) == fg);   // the stem of the I
    CHECK(pd::host::pixel(2, 2) == bg);
    CHECK(pd::host::pixel(317, 317) == bg);
    abc::draw_big('O', fg, bg);
    CHECK(pd::host::pixel(160, 160) == bg);   // the hole of the O
    // The O fills the height: ink near the top and bottom on the centre line.
    bool top = false, bottom = false;
    for (int y = 0; y < 20; ++y) top |= pd::host::pixel(160, y) == fg;
    for (int y = 300; y < 320; ++y) bottom |= pd::host::pixel(160, y) == fg;
    CHECK(top && bottom);
    // Scaled down (the menu): ink only inside the glyph's box.
    pd::gfx::clear(bg);
    abc::Glyph g;
    CHECK(abc::glyph('A', g));
    const int w = abc::glyph_width(g, 50);
    abc::draw_glyph(g, 100, 100, 50, fg, bg, 100, w);
    int ink = 0, outside = 0;
    for (int y = 90; y < 160; ++y)
        for (int x = 90; x < 100 + w + 10; ++x) {
            const bool in = x >= 100 && x < 100 + w && y >= 100 && y < 150;
            if (pd::host::pixel(x, y) != bg) (in ? ink : outside)++;
        }
    CHECK(ink > 200 && outside == 0);
}

}  // namespace

int main() {
    static Capture cap;
    pd::host::set_audio_sink(&cap);
    pd::Terminal& t = pd::terminal_init();
    pd::gfx::init(t.display());
    test_assets();
    test_sound(cap);
    test_draw();
    if (failures) {
        fprintf(stderr, "podabc_test: %d failures\n", failures);
        return 1;
    }
    printf("podabc_test: ok\n");
    return 0;
}
