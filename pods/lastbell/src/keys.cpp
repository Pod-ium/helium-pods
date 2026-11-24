// The Last Bell - raw key events, lower-cased like PodCraft's input.
#include "keys.h"

#include <cstring>

#include "pd/keys.h"

namespace lb {

namespace {
pd::input::Keyboard* g_kbd = nullptr;
bool g_down[256];
bool g_hit[256];
}  // namespace

void keys_attach(pd::input::Keyboard* k) { g_kbd = k; }

void poll_keys() {
    if (!g_kbd) return;
    pd::hal::RawKey ev;
    while (g_kbd->poll_raw(ev)) {
        const uint8_t k = pd::key::lower(ev.code);
        if (ev.state == pd::hal::KEY_RELEASED) {
            g_down[k] = false;
        } else {
            g_down[k] = true;
            if (ev.state == pd::hal::KEY_PRESSED) g_hit[k] = true;
        }
    }
}

bool key_down(uint8_t k) { return g_down[k]; }

bool key_hit(uint8_t k) {
    if (!g_hit[k]) return false;
    g_hit[k] = false;
    return true;
}

void clear_hits() { memset(g_hit, 0, sizeof g_hit); }

}  // namespace lb
