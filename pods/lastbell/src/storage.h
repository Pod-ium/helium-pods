// The Last Bell - saves on the SD card: SAVE_DIR/saveN.pqs, a 64-byte header
// (magic, version, checksum, slot summary) followed by the Game struct.
#pragma once
#include <cstddef>

#include "game.h"

namespace lb {

enum SaveErr { SAVE_OK = 0, SAVE_NO_CARD, SAVE_EMPTY, SAVE_IO_ERROR, SAVE_BAD_FILE };

int storage_save(int slot, const Game& g);
int storage_load(int slot, Game& g);
// Short description of a slot for the title screen ("Lv 4, 2 bells, 130G"),
// or "empty"; false when there is no card at all.
bool storage_slot_info(int slot, char* desc, size_t n);
constexpr int kSaveFileSize = 64 + int(sizeof(Game));

}  // namespace lb
