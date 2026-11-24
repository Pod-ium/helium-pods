// The Last Bell - turn-based battles. Enemies on the top half, a log window,
// the party status at the bottom; each living warden picks Attack / Skill /
// Item / Run, then the enemies act. Turn order is by agility each round.
#include "battle.h"

#include <algorithm>
#include <cstdio>
#include <string>
#include <vector>

#include "game.h"
#include "keys.h"
#include "pd/keys.h"
#include "pd/pd.h"
#include "render.h"
#include "rng.h"

namespace lb {
namespace {

constexpr int kLogX = 2, kLogY = 58, kLogW = 106, kLogH = 50;
constexpr int kStatX = 2, kStatY = 110, kStatW = 156, kStatH = 50;
constexpr int kCmdX = 110, kCmdY = 58, kCmdW = 48, kCmdH = 50;

struct Fighter {
    int enemy = 0;
    int hp = 0;
    bool alive = true;
    int dmg = 0;
    uint32_t dmg_until = 0;
    uint32_t flash_until = 0;
};

struct Battle {
    Game& g;
    Fighter f[4];
    int nf = 0;
    bool no_flee = false;
    std::vector<std::string> log;
    BattleResult result = BR_WIN;
    bool done = false;
    Battle(Game& gg, const EncGroup& grp, bool noflee) : g(gg), no_flee(noflee) {
        nf = grp.count > 4 ? 4 : int(grp.count);
        for (int i = 0; i < nf; ++i) {
            f[i].enemy = grp.enemies[i];
            f[i].hp = kEnemies[grp.enemies[i]].hp;
        }
    }
    void say(const std::string& s) {
        log.push_back(s);
        while (log.size() > 3) log.erase(log.begin());
    }
    bool won() const {
        for (int i = 0; i < nf; ++i) if (f[i].alive) return false;
        return true;
    }
    bool lost() const {
        for (int i = 0; i < PARTY_SIZE; ++i) if (!g.party[i].dead) return false;
        return true;
    }
};

// Enemy sprite positions for a group of n (area x 2..108, y 2..56).
void enemy_pos(int i, int n, int* x, int* y) {
    static const int8_t pos[4][4][2] = {
        {{44, 14}, {0, 0}, {0, 0}, {0, 0}},
        {{28, 18}, {64, 18}, {0, 0}, {0, 0}},
        {{20, 10}, {60, 10}, {40, 36}, {0, 0}},
        {{18, 8}, {58, 8}, {18, 34}, {58, 34}},
    };
    *x = pos[n - 1][i][0];
    *y = pos[n - 1][i][1];
}

void draw_base(Battle& b) {
    fb_clear(C_WIN_BG);
    const uint32_t now = pd::sys::now_ms();
    for (int i = 0; i < b.nf; ++i) {
        if (!b.f[i].alive) continue;
        int x, y;
        enemy_pos(i, b.nf, &x, &y);
        const EnemyDef& e = kEnemies[b.f[i].enemy];
        const bool flash = now < b.f[i].flash_until;
        if (b.f[i].enemy == EN_VEY) fb_sprite_scaled(x, y, e.sprite, 2, flash ? C_FLASH : 0);
        else fb_sprite(x, y, e.sprite, flash ? C_FLASH : 0);
        if (now < b.f[i].dmg_until && b.f[i].dmg != 0) {
            char buf[8];
            snprintf(buf, sizeof buf, "%d", b.f[i].dmg);
            fb_text(x + 4, y - 10, buf, C_YELLOW);
        }
    }
    win(kLogX, kLogY, kLogW, kLogH);
    for (size_t i = 0; i < b.log.size() && i < 3; ++i)
        fb_text(kLogX + 4, kLogY + 4 + int(i) * 12, b.log[i], C_WIN_FG);
    win(kStatX, kStatY, kStatW, kStatH);
    for (int i = 0; i < PARTY_SIZE; ++i) {
        const Member& m = b.g.party[i];
        const int cx = kStatX + 4;
        const int cy = kStatY + 3 + i * 12;
        char line[48];
        if (m.dead)
            snprintf(line, sizeof line, "%s Lv%d DEAD", m.name, m.level);
        else
            snprintf(line, sizeof line, "%s Lv%d HP %d/%d MP %d/%d", m.name, m.level, m.hp, m_hpmax(m),
                     m.mp, m_mpmax(m));
        fb_text(cx, cy, line, m.dead ? C_RED : (m.hp <= m_hpmax(m) / 4 ? C_YELLOW : C_WIN_FG));
    }
    render_submit();
}

void draw_cmd(Battle& b, int sel) {
    draw_base(b);
    win(kCmdX, kCmdY, kCmdW, kCmdH);
    static const char* const cmds[4] = {"Attack", "Skill", "Item", "Run"};
    for (int i = 0; i < 4; ++i) {
        if (i == sel) fb_fill(kCmdX + 2, kCmdY + 3 + i * 12, kCmdW - 4, 11, C_SEL);
        fb_text(kCmdX + 6, kCmdY + 4 + i * 12, cmds[i], i == sel ? C_WIN_FG : C_DIM);
    }
    render_submit();
}

void draw_list(Battle& b, const std::vector<std::string>& rows, int sel) {
    draw_base(b);
    const int w = kCmdW + 56, h = int(rows.size()) * 12 + 8;
    const int x = kCmdX + kCmdW - w, y = kCmdY + kCmdH - h;
    win(x, y, w, h);
    for (size_t i = 0; i < rows.size(); ++i) {
        if (int(i) == sel) fb_fill(x + 2, y + 3 + int(i) * 12, w - 4, 11, C_SEL);
        fb_text(x + 6, y + 4 + int(i) * 12, rows[i], int(i) == sel ? C_WIN_FG : C_DIM);
    }
    render_submit();
}

// Redraws the base screen for ms milliseconds (hit flashes, damage numbers),
// discarding keys; Enter skips early.
void wait_anim(Battle& b, uint32_t ms) {
    const uint32_t end = pd::sys::now_ms() + ms;
    while (pd::sys::now_ms() < end) {
        poll_keys();
        if (key_hit(pd::key::Enter) || key_hit(pd::key::Space)) break;
        draw_base(b);
        pd::sys::sleep_ms(30);
    }
    clear_hits();
}

void wait_key(Battle& b) {
    for (;;) {
        poll_keys();
        if (key_hit(pd::key::Enter) || key_hit(pd::key::Space) || key_hit(pd::key::Esc)) break;
        draw_base(b);
        pd::sys::sleep_ms(20);
    }
    clear_hits();
}

int phys_damage(uint32_t& rng, int atk, int def) {
    int base = 2 * atk - def;
    if (base < 1) base = 1;
    return base * (75 + rnd(rng, 50)) / 100;
}

int skill_damage(uint32_t& rng, int power, int level, int def) {
    int base = power + 2 * level - def / 2;
    if (base < 1) base = 1;
    return base * (75 + rnd(rng, 50)) / 100;
}

void hit_fighter(Battle& b, int i, int dmg) {
    Fighter& f = b.f[i];
    f.hp -= dmg;
    if (f.hp <= 0) { f.hp = 0; f.alive = false; }
    f.dmg = dmg;
    f.dmg_until = pd::sys::now_ms() + 600;
    f.flash_until = pd::sys::now_ms() + 250;
}

void hit_member(Battle& b, int mi, int dmg) {
    Member& m = b.g.party[mi];
    m.hp -= dmg;
    if (m.hp <= 0) { m.hp = 0; m.dead = 1; }
}

// Cursor over living enemies. Returns the index, or -1 on Esc.
int pick_enemy(Battle& b, int start) {
    int t = start;
    for (int k = 0; k < b.nf && !b.f[t].alive; ++k) t = (t + 1) % b.nf;
    for (;;) {
        poll_keys();
        if (key_hit(pd::key::Left) || key_hit(pd::key::Right) || key_hit(pd::key::Up) || key_hit(pd::key::Down)) {
            int nt = t;
            do { nt = (nt + 1) % b.nf; } while (!b.f[nt].alive);
            t = nt;
        }
        if (key_hit(pd::key::Enter) || key_hit(pd::key::Space)) { clear_hits(); return t; }
        if (key_hit(pd::key::Esc)) { clear_hits(); return -1; }
        // redraw with cursor
        draw_base(b);
        int x, y;
        enemy_pos(t, b.nf, &x, &y);
        fb_text(x - 8, y + 4, ">", C_YELLOW);
        render_submit();
        pd::sys::sleep_ms(20);
    }
}

// Cursor over party members (optionally only the dead, for Phoenix Down).
int pick_ally(Battle& b, int start, bool only_dead) {
    int t = start;
    auto ok = [&](int i) { return !only_dead || b.g.party[i].dead; };
    for (int k = 0; k < PARTY_SIZE && !ok(t); ++k) t = (t + 1) % PARTY_SIZE;
    for (;;) {
        poll_keys();
        if (key_hit(pd::key::Left) || key_hit(pd::key::Up)) { int nt = t; do { nt = (nt + 3) % PARTY_SIZE; } while (!ok(nt)); t = nt; }
        if (key_hit(pd::key::Right) || key_hit(pd::key::Down)) { int nt = t; do { nt = (nt + 1) % PARTY_SIZE; } while (!ok(nt)); t = nt; }
        if (key_hit(pd::key::Enter) || key_hit(pd::key::Space)) { clear_hits(); return t; }
        if (key_hit(pd::key::Esc)) { clear_hits(); return -1; }
        draw_base(b);
        const int cx = kStatX + 4, cy = kStatY + 3 + t * 12;
        fb_text(cx - 8, cy, ">", C_YELLOW);
        render_submit();
        pd::sys::sleep_ms(20);
    }
}

bool try_flee(Battle& b, int mi) {
    if (b.no_flee) { b.say("No escape!"); return false; }
    const Member& m = b.g.party[mi];
    int eagi = 0;
    for (int i = 0; i < b.nf; ++i) if (b.f[i].alive) eagi += kEnemies[b.f[i].enemy].agi;
    eagi /= b.nf > 0 ? b.nf : 1;
    int chance = 50 + (m_agi(m) - eagi) * 4;
    chance = std::clamp(chance, 10, 95);
    if (rnd(b.g.rng, 100) < chance) {
        b.say("Got away safely!");
        b.result = BR_FLEE;
        b.done = true;
        return true;
    }
    b.say("Can't escape!");
    return false;
}

void member_turn(Battle& b, int mi) {
    Member& m = b.g.party[mi];
    int sel = 0;
    for (;;) {
        draw_cmd(b, sel);
        poll_keys();
        if (key_hit(pd::key::Up)) sel = (sel + 3) % 4;
        if (key_hit(pd::key::Down)) sel = (sel + 1) % 4;
        if (!key_hit(pd::key::Enter) && !key_hit(pd::key::Space)) {
            pd::sys::sleep_ms(15);
            continue;
        }
        clear_hits();
        if (sel == 0) {   // ---- Attack
            const int t = pick_enemy(b, 0);
            if (t < 0) continue;
            const int dmg = phys_damage(b.g.rng, m_atk(m), kEnemies[b.f[t].enemy].def);
            hit_fighter(b, t, dmg);
            b.say(std::string(m.name) + " attacks! " + std::to_string(dmg) + " damage!");
            wait_anim(b, 450);
            return;
        }
        if (sel == 1) {   // ---- Skill
            std::vector<int> sks;
            for (int s = 0; s < SK_COUNT; ++s) if (m_has(m, s)) sks.push_back(s);
            if (sks.empty()) { b.say("No skills learned yet."); wait_anim(b, 300); continue; }
            std::vector<std::string> rows;
            for (int s : sks) {
                char buf[32];
                snprintf(buf, sizeof buf, "%s %dMP", kSkills[s].name, kSkills[s].mp);
                rows.push_back(buf);
            }
            int ssel = 0;
            bool chosen = false;
            int sk = -1;
            while (!chosen) {
                draw_list(b, rows, ssel);
                poll_keys();
                if (key_hit(pd::key::Up)) ssel = (ssel + int(sks.size()) - 1) % int(sks.size());
                if (key_hit(pd::key::Down)) ssel = (ssel + 1) % int(sks.size());
                if (key_hit(pd::key::Esc)) { clear_hits(); break; }
                if (key_hit(pd::key::Enter) || key_hit(pd::key::Space)) { sk = sks[ssel]; chosen = true; }
                else pd::sys::sleep_ms(15);
            }
            clear_hits();
            if (sk < 0) continue;
            const SkillDef& sd = kSkills[sk];
            if (m.mp < sd.mp) { b.say("Not enough MP!"); wait_anim(b, 300); continue; }
            m.mp -= sd.mp;
            if (sk == SK_VANISH) {
                if (try_flee(b, mi)) { wait_anim(b, 350); return; }
                wait_anim(b, 350);
                return;
            }
            if (sk == SK_STEAL) {
                const int t = pick_enemy(b, 0);
                if (t < 0) { m.mp += sd.mp; continue; }
                const int gold = 20 + rnd(b.g.rng, 40) + kEnemies[b.f[t].enemy].gold / 2;
                b.g.gold += gold;
                b.say(std::string(m.name) + " stole " + std::to_string(gold) + "G!");
                wait_anim(b, 400);
                return;
            }
            if (sd.target == TGT_ONE_ENEMY || sd.target == TGT_ALL_ENEMIES) {
                int t = 0;
                if (sd.target == TGT_ONE_ENEMY) {
                    t = pick_enemy(b, 0);
                    if (t < 0) { m.mp += sd.mp; continue; }
                }
                int total = 0;
                for (int i = 0; i < b.nf; ++i) {
                    if (!b.f[i].alive) continue;
                    if (sd.target == TGT_ONE_ENEMY && i != t) continue;
                    const int dmg = skill_damage(b.g.rng, sd.power, m.level, kEnemies[b.f[i].enemy].def);
                    hit_fighter(b, i, dmg);
                    total += dmg;
                }
                b.say(std::string(m.name) + " casts " + sd.name + "! " + std::to_string(total) + " damage!");
                wait_anim(b, 450);
                return;
            }
            // ally healing
            if (sd.target == TGT_ONE_ALLY) {
                int t = pick_ally(b, mi, false);
                if (t < 0) { m.mp += sd.mp; continue; }
                Member& tm = b.g.party[t];
                const int amt = sd.power + 3 * m.level;
                tm.hp = std::min(m_hpmax(tm), tm.hp + amt);
                b.say(std::string(m.name) + " casts " + sd.name + ": " + tm.name + " +" + std::to_string(amt) + " HP!");
                wait_anim(b, 400);
                return;
            }
            {   // TGT_ALL_ALLIES
                int amt = sd.power + 3 * m.level;
                for (int i = 0; i < PARTY_SIZE; ++i) {
                    Member& tm = b.g.party[i];
                    if (!tm.dead) tm.hp = std::min(m_hpmax(tm), tm.hp + amt);
                }
                b.say(std::string(m.name) + " casts " + sd.name + ": all +" + std::to_string(amt) + " HP!");
                wait_anim(b, 450);
                return;
            }
        }
        if (sel == 2) {   // ---- Item
            std::vector<std::string> rows;
            std::vector<int> ids;
            for (int it = 0; it < IT_COUNT; ++it)
                if (b.g.items[it] > 0) {
                    char buf[40];
                    snprintf(buf, sizeof buf, "%s x%d", kItems[it].name, b.g.items[it]);
                    rows.push_back(buf);
                    ids.push_back(it);
                }
            if (rows.empty()) { b.say("No items!"); wait_anim(b, 300); continue; }
            int isel = 0;
            bool chosen = false;
            int it = -1;
            while (!chosen) {
                draw_list(b, rows, isel);
                poll_keys();
                if (key_hit(pd::key::Up)) isel = (isel + int(rows.size()) - 1) % int(rows.size());
                if (key_hit(pd::key::Down)) isel = (isel + 1) % int(rows.size());
                if (key_hit(pd::key::Esc)) { clear_hits(); break; }
                if (key_hit(pd::key::Enter) || key_hit(pd::key::Space)) { it = ids[isel]; chosen = true; }
                else pd::sys::sleep_ms(15);
            }
            clear_hits();
            if (it < 0) continue;
            const ItemDef& idf = kItems[it];
            const int t = pick_ally(b, mi, idf.revive);
            if (t < 0) continue;
            Member& tm = b.g.party[t];
            if (idf.revive) {
                if (!tm.dead) continue;
                tm.dead = 0;
                tm.hp = int16_t(m_hpmax(tm) * idf.power / 100);
                b.say(tm.name + std::string(" is revived!"));
            } else if (it == IT_POTION) {
                const int amt = std::min(m_hpmax(tm) - tm.hp, idf.power);
                tm.hp += amt;
                b.say(tm.name + std::string(" recovered ") + std::to_string(amt) + " HP!");
            } else {
                const int amt = std::min(m_mpmax(tm) - tm.mp, idf.power);
                tm.mp += amt;
                b.say(tm.name + std::string(" recovered ") + std::to_string(amt) + " MP!");
            }
            b.g.items[it]--;
            wait_anim(b, 400);
            return;
        }
        // ---- Run
        try_flee(b, mi);
        wait_anim(b, 400);
        return;
    }
}

void enemy_turn(Battle& b, int fi) {
    const EnemyDef& e = kEnemies[b.f[fi].enemy];
    int t = rnd(b.g.rng, PARTY_SIZE);
    for (int k = 0; k < PARTY_SIZE && b.g.party[t].dead; ++k) t = (t + 1) % PARTY_SIZE;
    if (b.g.party[t].dead) return;
    if (e.cast >= 0 && rnd(b.g.rng, 4) == 0) {
        const SkillDef& sd = kSkills[e.cast];
        const int dmg = skill_damage(b.g.rng, sd.power, e.atk / 2, m_def(b.g.party[t]));
        hit_member(b, t, dmg);
        b.say(std::string(e.name) + " casts " + sd.name + "! " + std::to_string(dmg) + " damage!");
    } else {
        const int dmg = phys_damage(b.g.rng, e.atk, m_def(b.g.party[t]));
        hit_member(b, t, dmg);
        b.say(std::string(e.name) + " attacks " + b.g.party[t].name + "! " + std::to_string(dmg) + " damage!");
    }
    wait_anim(b, 450);
}

}  // namespace

BattleResult run_battle(Game& g, const EncGroup& grp, bool no_flee) {
    Battle b(g, grp, no_flee);
    printf("[LB] battle start:");
    for (int i = 0; i < b.nf; ++i) printf(" %s", kEnemies[b.f[i].enemy].name);
    printf("\n");
    // intro: who appeared
    for (int i = 0; i < b.nf; ++i) {
        bool seen = false;
        for (int j = 0; j < i; ++j) if (b.f[j].enemy == b.f[i].enemy) seen = true;
        if (!seen) b.say(std::string(kEnemies[b.f[i].enemy].name) + " appeared!");
    }
    draw_base(b);
    wait_key(b);

    while (!b.done) {
        if (b.won()) {
            int exp = 0, gold = 0;
            for (int i = 0; i < b.nf; ++i) { exp += kEnemies[b.f[i].enemy].exp; gold += kEnemies[b.f[i].enemy].gold; }
            b.say("Victory!");
            draw_base(b);
            char buf[40];
            snprintf(buf, sizeof buf, "%d EXP, %dG obtained", exp, gold);
            win(kLogX, kLogY, kLogW, kLogH + 14);
            for (size_t i = 0; i < b.log.size() && i < 3; ++i)
                fb_text(kLogX + 4, kLogY + 4 + int(i) * 12, b.log[i], C_WIN_FG);
            fb_text(kLogX + 4, kLogY + 40, buf, C_YELLOW);
            render_submit();
            printf("[LB] victory exp=%d gold=%d\n", exp, gold);
            wait_key(b);
            g.gold += gold;
            gain_exp(g, exp);
            b.result = BR_WIN;
            b.done = true;
            break;
        }
        if (b.lost()) {
            b.say("The party has fallen...");
            draw_base(b);
            printf("[LB] game over\n");
            wait_key(b);
            b.result = BR_LOSE;
            b.done = true;
            break;
        }
        // turn order by agility
        struct T { bool member; int idx; int agi; };
        std::vector<T> order;
        for (int i = 0; i < PARTY_SIZE; ++i)
            if (!g.party[i].dead) order.push_back({true, i, m_agi(g.party[i])});
        for (int i = 0; i < b.nf; ++i)
            if (b.f[i].alive) order.push_back({false, i, kEnemies[b.f[i].enemy].agi});
        std::stable_sort(order.begin(), order.end(), [](const T& a, const T& c) { return a.agi > c.agi; });
        for (const T& t : order) {
            if (b.done || b.won() || b.lost()) break;
            if (t.member) member_turn(b, t.idx);
            else enemy_turn(b, t.idx);
        }
    }
    wait_anim(b, 200);
    return b.result;
}

}  // namespace lb
