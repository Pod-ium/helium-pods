// The Last Bell - game state (a POD, saved verbatim) and party logic.
#pragma once
#include <cstdint>

#include "config.h"
#include "data.h"

namespace lb {

enum Flag : uint32_t {
    F_QUEST  = 1u << 0,
    F_BELL0  = 1u << 1,
    F_BELL1  = 1u << 2,
    F_BELL2  = 1u << 3,
    F_BELL3  = 1u << 4,
    F_VEY    = 1u << 5,
    F_WON    = 1u << 6,
    F_G0     = 1u << 7,    // tower guardians defeated (bits 7..10)
};
constexpr uint32_t flag_bell(int b) { return F_BELL0 << b; }
constexpr uint32_t flag_guard(int b) { return F_G0 << b; }
inline int popcount_bells(uint32_t f) {
    int n = 0;
    for (int b = 0; b < 4; ++b) n += (f & flag_bell(b)) ? 1 : 0;
    return n;
}

struct Member {
    uint8_t cls;
    char name[8];
    uint8_t level;
    uint8_t dead;
    uint16_t skills;      // bitmask of SkillId
    uint32_t exp;
    int16_t hp, mp;       // maxima are derived from class + level
};

struct Game {
    Member party[PARTY_SIZE];
    int32_t gold;
    uint8_t items[IT_COUNT];
    uint8_t map;
    int16_t px, py;
    uint8_t dir;          // 0 down, 1 up, 2 left, 3 right
    uint8_t frame;        // walk animation
    uint32_t flags;
    uint32_t rng;
    uint32_t steps;
    uint32_t play_ms;
    uint8_t chests;       // bitmask of opened chests
    uint8_t pad[3];
};

void new_game(Game& g);

int m_hpmax(const Member& m);
int m_mpmax(const Member& m);
int m_atk(const Member& m);
int m_def(const Member& m);
int m_agi(const Member& m);
bool m_has(const Member& m, int skill);
void gain_exp(Game& g, int exp);     // splits among the living, levels up
void heal_all(Game& g);

}  // namespace lb
