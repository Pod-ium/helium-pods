// The Last Bell - party state and levelling.
#include "game.h"

#include <cstdio>
#include <cstring>

#include "maps.h"
#include "rng.h"

namespace lb {

void new_game(Game& g) {
    memset(&g, 0, sizeof g);
    static const struct { int cls; const char* name; } kParty[PARTY_SIZE] = {
        {CLS_WARRIOR, "Bram"}, {CLS_WHITE, "Sylva"}, {CLS_BLACK, "Nyx"}, {CLS_THIEF, "Pip"},
    };
    for (int i = 0; i < PARTY_SIZE; ++i) {
        Member& m = g.party[i];
        m.cls = uint8_t(kParty[i].cls);
        snprintf(m.name, sizeof m.name, "%s", kParty[i].name);
        m.level = 1;
        m.exp = 0;
        m.skills = 0;
        m.dead = 0;
        m.hp = int16_t(m_hpmax(m));
        m.mp = int16_t(m_mpmax(m));
    }
    g.gold = 20;
    g.items[IT_POTION] = 0;
    g.items[IT_ETHER] = 0;
    g.items[IT_PHOENIX] = 0;
    g.map = MAP_QUAY;
    g.px = 8; g.py = 14;
    g.dir = 0;
    g.flags = 0;
    g.rng = 0x1BADB002u;   // fixed seed: new games (and the scripted test) replay exactly
    g.steps = 0;
    g.play_ms = 0;
    g.chests = 0;
    printf("[LB] new game: 4 wardens leave Quayhaven\n");
}

int m_hpmax(const Member& m) { const ClassDef& c = kClasses[m.cls]; return c.hp + c.hp_g * (m.level - 1); }
int m_mpmax(const Member& m) { const ClassDef& c = kClasses[m.cls]; return c.mp + c.mp_g * (m.level - 1); }
int m_atk(const Member& m)   { const ClassDef& c = kClasses[m.cls]; return c.atk + c.atk_g * (m.level - 1); }
int m_def(const Member& m)   { const ClassDef& c = kClasses[m.cls]; return c.def + c.def_g * (m.level - 1); }
int m_agi(const Member& m)   { const ClassDef& c = kClasses[m.cls]; return c.agi + c.agi_g * (m.level - 1); }
bool m_has(const Member& m, int skill) { return (m.skills >> skill) & 1; }

void gain_exp(Game& g, int exp) {
    int alive = 0;
    for (int i = 0; i < PARTY_SIZE; ++i) alive += g.party[i].dead ? 0 : 1;
    if (alive == 0) return;
    const int share = exp / alive > 0 ? exp / alive : 1;
    for (int i = 0; i < PARTY_SIZE; ++i) {
        Member& m = g.party[i];
        if (m.dead) continue;
        m.exp += uint32_t(share);
        while (m.level < MAX_LEVEL && m.exp >= uint32_t(exp_needed(m.level))) {
            ++m.level;
            m.hp = int16_t(m_hpmax(m));   // levelling restores to full
            m.mp = int16_t(m_mpmax(m));
            printf("[LB] %s reached level %d\n", m.name, m.level);
            for (int l = 0; l < kLearnCount[m.cls]; ++l)
                if (kLearn[m.cls][l].level == m.level) {
                    m.skills |= uint16_t(1u << kLearn[m.cls][l].skill);
                    printf("[LB] %s learned %s\n", m.name, kSkills[kLearn[m.cls][l].skill].name);
                }
        }
    }
}

void heal_all(Game& g) {
    for (int i = 0; i < PARTY_SIZE; ++i) {
        Member& m = g.party[i];
        m.dead = 0;
        m.hp = int16_t(m_hpmax(m));
        m.mp = int16_t(m_mpmax(m));
    }
}

}  // namespace lb
