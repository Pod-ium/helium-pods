// The Last Bell - save files: 64-byte header + the Game struct, checksummed.
#include "storage.h"

#include <cstdio>
#include <cstring>

#include "config.h"
#include "maps.h"
#include "pd/fs.h"

namespace lb {

namespace {

struct SaveHdr {
    char magic[4];        // SAVE_MAGIC
    uint32_t version;
    uint32_t payload;     // sizeof(Game)
    uint32_t checksum;    // sum of the payload bytes
    uint8_t slot;
    uint8_t bells;
    uint8_t level;        // highest party level
    uint8_t pad;
    uint32_t gold;
    char where[16];       // map name
    uint8_t reserved[24];
};
static_assert(sizeof(SaveHdr) == 64, "save header is 64 bytes");

bool mount() { return pd::fs::mounted() || pd::fs::mount(); }

void path(char* buf, size_t n, int slot) { snprintf(buf, n, SAVE_DIR "/save%d.pqs", slot + 1); }

uint32_t checksum(const Game& g) {
    const uint8_t* p = reinterpret_cast<const uint8_t*>(&g);
    uint32_t s = 0;
    for (size_t i = 0; i < sizeof g; ++i) s += p[i];
    return s;
}

}  // namespace

int storage_save(int slot, const Game& g) {
    if (!mount()) return SAVE_NO_CARD;
    pd::fs::mkdirs(SAVE_DIR);
    char p[40];
    path(p, sizeof p, slot);
    SaveHdr h;
    memset(&h, 0, sizeof h);
    memcpy(h.magic, SAVE_MAGIC, 4);
    h.version = SAVE_VERSION;
    h.payload = sizeof(Game);
    h.checksum = checksum(g);
    h.slot = uint8_t(slot);
    h.bells = uint8_t(popcount_bells(g.flags));
    int lvl = 1;
    for (int i = 0; i < PARTY_SIZE; ++i) lvl = g.party[i].level > lvl ? g.party[i].level : lvl;
    h.level = uint8_t(lvl);
    h.gold = uint32_t(g.gold);
    snprintf(h.where, sizeof h.where, "%s", map_def(g.map).name);
    pd::fs::File f;
    if (!f.open(p, pd::fs::Mode::Write)) {
        pd::fs::unmount();
        return SAVE_IO_ERROR;
    }
    const bool ok = f.write(&h, sizeof h) == int(sizeof h) && f.write(&g, sizeof g) == int(sizeof g);
    f.close();
    if (ok) printf("[LB] saved slot %d (%s)\n", slot + 1, h.where);
    return ok ? SAVE_OK : SAVE_IO_ERROR;
}

int storage_load(int slot, Game& g) {
    if (!mount()) return SAVE_NO_CARD;
    char p[40];
    path(p, sizeof p, slot);
    if (!pd::fs::exists(p)) return SAVE_EMPTY;
    pd::fs::File f;
    if (!f.open(p, pd::fs::Mode::Read)) return SAVE_IO_ERROR;
    if (f.size() != kSaveFileSize) return SAVE_BAD_FILE;
    SaveHdr h;
    if (f.read(&h, sizeof h) != int(sizeof h)) return SAVE_IO_ERROR;
    if (memcmp(h.magic, SAVE_MAGIC, 4) != 0 || h.version != SAVE_VERSION || h.payload != sizeof(Game))
        return SAVE_BAD_FILE;
    if (f.read(&g, sizeof g) != int(sizeof g)) return SAVE_IO_ERROR;
    if (checksum(g) != h.checksum) return SAVE_BAD_FILE;
    printf("[LB] loaded slot %d (%s)\n", slot + 1, h.where);
    return SAVE_OK;
}

bool storage_slot_info(int slot, char* desc, size_t n) {
    if (!mount()) return false;
    char p[40];
    path(p, sizeof p, slot);
    if (!pd::fs::exists(p)) { snprintf(desc, n, "empty"); return true; }
    pd::fs::File f;
    if (!f.open(p, pd::fs::Mode::Read)) { snprintf(desc, n, "unreadable"); return true; }
    SaveHdr h;
    if (f.size() != kSaveFileSize || f.read(&h, sizeof h) != int(sizeof h) ||
        memcmp(h.magic, SAVE_MAGIC, 4) != 0) {
        snprintf(desc, n, "corrupt");
        return true;
    }
    snprintf(desc, n, "Lv %d, %d/4 bells, %uG, %s", h.level, h.bells, h.gold, h.where);
    return true;
}

}  // namespace lb
