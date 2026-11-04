// PodABC - letters and numbers for young children, in Italian.
//
// Two games, picked from a big two-button menu:
//   1 "Ascolta e trova"  the PicoCalc says a letter or a number; the child
//                        presses its key. Right: the symbol fills the screen,
//                        a jingle and a "well done". Wrong: "Prova ancora!"
//                        and the symbol again. Enter or Space repeats it.
//   2 "Premi e ascolta"  every letter or number key pressed is shown on the
//                        whole screen and said aloud.
// Esc and Up held together go back to the menu (hard to hit by chance).
// Settings (for the grown-ups): which symbols game 1 asks, volume, background.
// They are kept on the SD card when there is one; the game does not need it.
#include <cstdio>

#include "assets.h"
#include "draw.h"
#include "pd/pd.h"
#include "pd/settings.h"
#include "sound.h"

namespace {

using abc::Sound;
namespace key = pd::key;

pd::input::Keyboard* kbd = nullptr;
Sound snd;

// ------------------------------------------------------------------ settings
enum SymbolSet { SET_ALL = 0, SET_LETTERS = 1, SET_DIGITS = 2 };
struct Prefs {
    int set = SET_ALL;
    int volume = 70;
    int dark = 0;
};
Prefs prefs;
constexpr const char* kPrefsDir = "/podabc";
constexpr const char* kPrefsPath = "/podabc/settings.txt";

bool card() { return pd::fs::mounted() || pd::fs::mount(); }

void load_prefs() {
    pd::Settings s;
    if (!card() || !s.load(kPrefsPath)) return;
    prefs.set = s.get_int("symbols", prefs.set, 0, 2);
    prefs.volume = s.get_int("volume", prefs.volume, 0, 100);
    prefs.dark = s.get_int("dark", prefs.dark, 0, 1);
}

void save_prefs() {
    if (!card()) return;
    pd::Settings s;
    s.set_int("symbols", prefs.set);
    s.set_int("volume", prefs.volume);
    s.set_int("dark", prefs.dark);
    pd::fs::mkdirs(kPrefsDir);
    if (!s.save(kPrefsPath)) pd::fs::unmount();   // card swapped? mount again next time
}

// ------------------------------------------------------------------ colours
pd::Color bg() { return prefs.dark ? 0x101828 : 0xFFFBF0; }
pd::Color ink() { return prefs.dark ? 0xF2F2F2 : 0x202830; }
pd::Color dim() { return prefs.dark ? 0x7A8494 : 0x9A9A9A; }
pd::Color button_bg() { return prefs.dark ? 0x1E2A40 : 0xFFFFFF; }
pd::Color button_sel() { return prefs.dark ? 0x2F4A7A : 0xFFE9A8; }
pd::Color frame_sel() { return prefs.dark ? 0xFFD24A : 0xF08C00; }

// Each symbol keeps its colour, so a child can remember it by colour too.
constexpr pd::Color kPalette[] = {0xE53935, 0xFB8C00, 0x43A047, 0x00ACC1,
                                  0x1E88E5, 0x5E35B1, 0x8E24AA, 0xD81B60};
pd::Color symbol_color(char c) {
    const int i = (c >= '0' && c <= '9') ? 26 + (c - '0') : (pd::key::lower(uint8_t(c)) - 'a');
    return kPalette[(i < 0 ? 0 : i) % 8];
}

bool is_symbol(uint8_t c) {
    c = key::lower(c);
    return (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9');
}

// ------------------------------------------------------------------ helpers
uint32_t rng_state = 0x2545F491u;
uint32_t rnd() {
    uint32_t x = rng_state;
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    return rng_state = x;
}
void reseed() { rng_state ^= uint32_t(pd::sys::now_us()) * 2654435761u | 1u; }

void pump() { snd.pump(); }

void idle() {
    snd.pump();
    pd::sys::sleep_ms(1);
}

// The clip that says symbol c.
const char* say(char c) {
    static char name[2];
    name[0] = char(key::lower(uint8_t(c)));
    name[1] = 0;
    return name;
}

enum class In { None, Back, Key };
struct Input {
    In what = In::None;
    uint8_t code = 0;
};

// Next key press, or Back for Esc + Up held together.
Input read_input() {
    pd::input::RawKey k;
    while (kbd->poll_raw(k)) {
        if (k.state != pd::hal::KEY_PRESSED) continue;
        if ((k.code == key::Up && kbd->down(key::Esc)) || (k.code == key::Esc && kbd->down(key::Up)))
            return {In::Back, k.code};
        return {In::Key, k.code};
    }
    return {};
}

// Lets the queued sound finish, then waits `after_ms` more. False if the
// child went back to the menu meanwhile (other keys are dropped).
bool wait_sound(uint32_t after_ms) {
    uint32_t until = 0;
    for (;;) {
        idle();
        if (read_input().what == In::Back) return false;
        if (snd.busy()) continue;
        if (!until) until = pd::sys::now_ms() + after_ms;
        if (pd::sys::reached(until)) return true;
    }
}

// ------------------------------------------------------------------ game 1
// The symbols game 1 asks, shuffled; each comes once before any repeats.
struct Bag {
    char items[36];
    int n = 0, next = 0;
    char last = 0;

    void fill() {
        n = 0;
        if (prefs.set != SET_DIGITS)
            for (char c = 'a'; c <= 'z'; ++c) items[n++] = c;
        if (prefs.set != SET_LETTERS)
            for (char c = '0'; c <= '9'; ++c) items[n++] = c;
        for (int i = n - 1; i > 0; --i) {
            const int j = int(rnd() % uint32_t(i + 1));
            const char t = items[i];
            items[i] = items[j];
            items[j] = t;
        }
        // not the same symbol twice in a row across two rounds
        if (n > 1 && items[0] == last) {
            items[0] = items[n - 1];
            items[n - 1] = last;
        }
        next = 0;
    }
    char take() {
        if (next >= n) fill();
        return last = items[next++];
    }
};

void game_find() {
    static const char* const kPraise[] = {"ottimo", "bene", "evviva", "esatto"};
    reseed();
    Bag bag;
    bag.fill();
    for (;;) {
        const char target = bag.take();
        snd.play(say(target));
        abc::draw_big('?', dim(), bg(), pump);

        // Waiting for the right key. Said again after a while without keys.
        uint32_t quiet_since = 0;
        int reminders = 0;
        for (;;) {
            idle();
            const Input in = read_input();
            if (in.what == In::Back) return;
            if (snd.busy()) {
                quiet_since = 0;
            } else if (!quiet_since) {
                quiet_since = pd::sys::now_ms();
            } else if (reminders < 3 && pd::sys::now_ms() - quiet_since > 9000) {
                snd.play(say(target));
                ++reminders;
            }
            if (in.what != In::Key) continue;
            const uint8_t c = in.code;
            if (c == key::Enter || c == key::Space) {   // "say it again"
                snd.play(say(target));
                continue;
            }
            if (!key::is_printable(c)) continue;   // arrows, Esc, function keys...
            reminders = 0;
            if (key::lower(c) == uint8_t(target)) break;
            snd.play("riprova");
            snd.then(say(target), 150);
        }

        // Right: the symbol, a jingle, a "well done".
        snd.play("jingle");
        snd.then(kPraise[rnd() % 4], 80);
        abc::draw_big(target, symbol_color(target), bg(), pump);
        if (!wait_sound(900)) return;
        kbd->flush();
    }
}

// ------------------------------------------------------------------ game 2
void game_say() {
    pd::gfx::clear(bg());
    const pd::gfx::Font& f = pd::gfx::font(pd::gfx::FONT_12x24);
    pd::gfx::text_centered(pd::gfx::height() / 2 - 12, f, "Premi un tasto!", ink(), bg());
    snd.play("premi");
    for (;;) {
        idle();
        Input in = read_input();
        if (in.what == In::Back) return;
        if (in.what != In::Key || !is_symbol(in.code)) continue;
        // Keys mashed while drawing: only the last one counts.
        for (Input more; (more = read_input()).what != In::None;) {
            if (more.what == In::Back) return;
            if (is_symbol(more.code)) in = more;
        }
        const char c = char(key::lower(in.code));
        snd.play(say(c));
        abc::draw_big(c, symbol_color(c), bg(), pump);
    }
}

// ------------------------------------------------------------------ settings
void settings(pd::ui::Context& ui) {
    static const char* const kSets[] = {"Lettere e numeri", "Solo lettere", "Solo numeri"};
    static const char* const kBg[] = {"Chiaro", "Scuro"};
    ui.th = prefs.dark ? &pd::ui::THEME_NIGHT : &pd::ui::THEME_PAPER;
    pd::ui::MenuItem items[] = {
        pd::ui::choice("Gioco 1 chiede", &prefs.set, kSets, 3),
        pd::ui::range("Volume", &prefs.volume, 0, 100, 10, "%d%%"),
        pd::ui::choice("Sfondo", &prefs.dark, kBg, 2),
        pd::ui::action("Indietro", "Esc"),
    };
    pd::ui::MenuOptions opt;
    opt.footer = "PodABC " PD_APP_VERSION;
    opt.on_change = [](int index) {
        if (index != 1) return;
        snd.set_volume(prefs.volume);
        snd.play("jingle");
    };
    kbd->flush();
    pd::ui::menu(ui, "IMPOSTAZIONI", items, opt);
    kbd->flush();
    snd.set_volume(prefs.volume);
    save_prefs();
}

// ------------------------------------------------------------------ menu
constexpr int kItems = 3;
struct Button {
    int x, y, w, h;
};
Button button(int i) {
    const int sw = pd::gfx::width();
    if (i < 2) return {16, 92 + i * 84, sw - 32, 74};
    return {16, 262, sw - 32, 34};
}

void draw_button(int i, bool sel) {
    static const char* const kTitle[] = {"Ascolta e trova", "Premi e ascolta", "Impostazioni"};
    static const char* const kHint[] = {"Senti il nome, trova il tasto", "Premi un tasto, senti il nome"};
    static const char kIcon[] = {'?', 'A'};
    const Button b = button(i);
    const pd::Color fill = sel ? button_sel() : button_bg();
    pd::gfx::fill(b.x, b.y, b.w, b.h, fill);
    pd::gfx::frame(b.x, b.y, b.w, b.h, sel ? frame_sel() : dim(), sel ? 3 : 1);
    if (i < 2) {
        abc::Glyph g;
        if (abc::glyph(kIcon[i], g)) {
            const int h = b.h - 20;
            const int w = abc::glyph_width(g, h);
            const pd::Color fg = i == 0 ? dim() : symbol_color('a');
            abc::draw_glyph(g, b.x + 12 + (48 - w) / 2, b.y + 10, h, fg, fill, b.x + 12 + (48 - w) / 2, w);
        }
        const pd::gfx::Font& big = pd::gfx::font(pd::gfx::FONT_12x24);
        const pd::gfx::Font& small = pd::gfx::font(pd::gfx::FONT_6x12);
        pd::gfx::text(b.x + 72, b.y + 14, 0, big, kTitle[i], ink(), fill);
        pd::gfx::text(b.x + 72, b.y + 44, 0, small, kHint[i], dim(), fill);
    } else {
        const pd::gfx::Font& f = pd::gfx::font(pd::gfx::FONT_8x16);
        const int tw = pd::gfx::text_cells(kTitle[i]) * f.w;
        pd::gfx::text(b.x + (b.w - tw) / 2, b.y + (b.h - f.h) / 2, 0, f, kTitle[i], ink(), fill);
    }
}

void draw_menu(int sel) {
    const int sw = pd::gfx::width(), sh = pd::gfx::height();
    pd::gfx::clear(bg());
    // Title: "ABC 123" in the symbols' own colours.
    const char* title = "ABC 123";
    const int h = 58, gap = 4, space = 22;
    int total = 0;
    for (const char* p = title; *p; ++p) {
        abc::Glyph g;
        total += (*p == ' ') ? space : (abc::glyph(*p, g) ? abc::glyph_width(g, h) + gap : 0);
    }
    int x = (sw - total + gap) / 2;
    for (const char* p = title; *p; ++p) {
        abc::Glyph g;
        if (*p == ' ' || !abc::glyph(*p, g)) {
            x += space;
            continue;
        }
        const int w = abc::glyph_width(g, h);
        abc::draw_glyph(g, x, 16, h, symbol_color(*p), bg(), x, w, pump);
        x += w + gap;
    }
    for (int i = 0; i < kItems; ++i) draw_button(i, i == sel);
    pd::gfx::text_centered(sh - 16, pd::gfx::font(pd::gfx::FONT_6x12), "Esc + Su insieme: torna qui", dim(),
                           bg());
}

// Returns the item picked: 0 = game 1, 1 = game 2, 2 = settings.
int main_menu(int sel) {
    draw_menu(sel);
    kbd->flush();
    for (;;) {
        idle();
        const Input in = read_input();
        if (in.what != In::Key) continue;
        int to = sel;
        switch (in.code) {
            case key::Up: to = (sel + kItems - 1) % kItems; break;
            case key::Down: to = (sel + 1) % kItems; break;
            case key::Enter:
            case key::Space: return sel;
            case '1': return 0;
            case '2': return 1;
            case '3': return 2;
            default: break;
        }
        if (to == sel) continue;
        draw_button(sel, false);
        draw_button(to, true);
        sel = to;
        if (sel < 2) snd.play(sel == 0 ? "trova" : "ascolta");   // the children cannot read yet
        else snd.stop();
    }
}

void fatal(const char* line) {
    pd::gfx::clear(0x000000);
    pd::gfx::text_centered(150, pd::gfx::font(pd::gfx::FONT_8x16), line, 0xFF6060, 0x000000);
    for (;;) pd::sys::sleep_ms(100);
}

}  // namespace

int main() {
    pd::TerminalConfig cfg;
    cfg.clock_khz = pd::board::fast_clock_khz();   // 200 MHz: finer PWM audio
    cfg.keyboard_on_core1 = true;                  // keys never stall the audio
    pd::Terminal& term = pd::terminal_init(cfg);
    pd::gfx::init(term.display());
    kbd = &term.keyboard();
    pd::ui::Context ui(term.keyboard(), pd::ui::THEME_PAPER);
    ui.idle = pump;

    if (!abc::assets_ok()) fatal("PodABC: dati audio/immagini mancanti");
    load_prefs();
    snd.init(term.audio(), prefs.volume);
    snd.play("ciao");

    int sel = 0;
    for (;;) {
        sel = main_menu(sel);
        if (sel == 0) game_find();
        else if (sel == 1) game_say();
        else settings(ui);
        snd.stop();
    }
    return 0;
}
