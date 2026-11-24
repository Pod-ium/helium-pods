// The Last Bell - raw keyboard state shared by the map loop, dialogs, menus
// and battles: held keys (movement) and one-shot presses (commands).
#pragma once
#include <cstdint>

#include "pd/input.h"

namespace lb {

void keys_attach(pd::input::Keyboard* k);
void poll_keys();
bool key_down(uint8_t k);
bool key_hit(uint8_t k);   // true once per press
void clear_hits();

}  // namespace lb
