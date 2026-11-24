// The Last Bell - an RPG for the PicoCalc, on the PicoPods libraries.
//
// Four Wardens of Quayhaven must re-ring the Four Bells of the drifting sky
// of Vael before the Hollow King Vey silences them all. Turn-based battles,
// skills learned on level-up, shops, inns and saves on the SD card.
//
// States: title -> map loop; dialogs, menus, shop/inn and battles are nested
// blocking loops that redraw the 160x160 indexed framebuffer and submit it
// through pd::IndexedBlitter (2x to the 320x320 panel).
#include <algorithm>
#include <cstdio>
#include <cstring>
#include <functional>
#include <string>
#include <vector>

#include "art.h"
#include "battle.h"
#include "config.h"
#include "data.h"
#include "game.h"
#include "keys.h"
#include "maps.h"
#include "pd/diag.h"
#include "pd/keys.h"
#include "pd/pd.h"
#include "render.h"
#include "rng.h"
#include "storage.h"

namespace {

using namespace lb;

const char* const kStages[] = {"start", "display", "art", "maps", "ready"};
enum Stage { kStart, kDisplay, kArt, kMaps, kReady };

Game g;

// Direction vectors: 0 down, 1 up, 2 left, 3 right.
const int kDx[4] = {0, 0, -1, 1};
const int kDy[4] = {1, -1, 0, 0};

// ---------------------------------------------------------------- dialog
// Typewriter dialog over the map (or a plain background on the title screen).
void run_dialog(const std::vector<std::string>& pages, bool map_bg = true) {
    std::vector<std::vector<std::string>> wrapped;
    for (const auto& p : pages) {
        auto w = wrap_text(p, 24);
        if (w.size() > 3) w.resize(3);
        wrapped.push_back(w);
    }
    size_t page = 0;
    int shown = 0;
    uint32_t last_type = 0;
    clear_hits();
    for (;;) {
        const auto& lines = wrapped[page];
        int total = 0;
        for (const auto& l : lines) total += int(l.size());
        const uint32_t now = pd::sys::now_ms();
        if (now - last_type >= TYPE_MS && shown < total) {
            ++shown;
            last_type = now;
        }
        if (map_bg) draw_map(g);
        else fb_clear(0);
        win(2, 114, 156, 44);
        int remaining = shown, y = 118;
        for (const auto& l : lines) {
            const int n = std::min<int>(int(l.size()), remaining);
            if (n > 0) fb_text(6, y, l.substr(0, n), C_WIN_FG);
            remaining -= n;
            y += 12;
        }
        if (shown >= total && (now / 300) % 2 == 0) fb_text(148, 148, "v", C_YELLOW);
        render_submit();
        poll_keys();
        if (key_hit(pd::key::Enter) || key_hit(pd::key::Space)) {
            if (shown < total) shown = total;
            else if (++page >= wrapped.size()) return;
            else shown = 0;
        }
        if (key_hit(pd::key::Esc)) {
            if (shown < total) shown = total;
            else return;
        }
        pd::sys::sleep_ms(10);
    }
}

void msg(const std::string& line, bool map_bg = true) { run_dialog({line}, map_bg); }

// ---------------------------------------------------------------- menus
// A centred list menu over a background painter; returns the index or -1 (Esc).
int list_menu(const std::function<void()>& bg, const char* title, const std::vector<std::string>& items,
              int initial = 0) {
    int sel = initial;
    int w = 8;
    for (const auto& it : items) w = std::max(w, 8 + int(it.size()) * 6);
    w = std::min(w, FB_W - 8);
    const int h = int(items.size()) * 12 + 20;
    const int x = (FB_W - w) / 2, y = (FB_H - h) / 2;
    clear_hits();
    for (;;) {
        bg();
        win(x, y, w, h);
        fb_text(x + 6, y + 5, title, C_YELLOW);
        for (size_t i = 0; i < items.size(); ++i) {
            if (int(i) == sel) fb_fill(x + 3, y + 17 + int(i) * 12, w - 6, 11, C_SEL);
            fb_text(x + 8, y + 18 + int(i) * 12, items[i], int(i) == sel ? C_WIN_FG : C_DIM);
        }
        render_submit();
        poll_keys();
        if (key_hit(pd::key::Up)) sel = (sel + int(items.size()) - 1) % int(items.size());
        if (key_hit(pd::key::Down)) sel = (sel + 1) % int(items.size());
        if (key_hit(pd::key::Enter) || key_hit(pd::key::Space)) { clear_hits(); return sel; }
        if (key_hit(pd::key::Esc)) { clear_hits(); return -1; }
        pd::sys::sleep_ms(15);
    }
}

// ---------------------------------------------------------------- help
std::vector<std::string> help_pages() {
    return {
        "Arrows: move and choose. Enter/Space: talk, confirm, open chests, ring bells.",
        "Esc: menu (status, items, save, quit). F1: this help.",
        "Tall grass and the dark isle hide monsters. Fight, learn skills, level up.",
        "Ring the Four Bells, then climb the Veilspire and face the Hollow King!",
    };
}

// ---------------------------------------------------------------- screens
void status_screen() {
    int sel = 0;
    clear_hits();
    for (;;) {
        draw_map(g);
        win(2, 6, 156, 148);
        fb_text(6, 10, "STATUS", C_YELLOW);
        for (int i = 0; i < PARTY_SIZE; ++i) {
            const Member& m = g.party[i];
            const int y = 26 + i * 24;
            if (i == sel) fb_fill(4, y - 2, 152, 22, C_SEL);
            char l1[40], l2[48];
            snprintf(l1, sizeof l1, "%s  Lv%d  %s", m.name, m.level, kClasses[m.cls].name);
            snprintf(l2, sizeof l2, "HP %d/%d  MP %d/%d  EXP %u", m.hp, m_hpmax(m), m.mp, m_mpmax(m), m.exp);
            fb_text(8, y, l1, C_WIN_FG);
            fb_text(8, y + 11, l2, m.dead ? C_RED : C_CYAN);
        }
        std::string sks = "Skills: ";
        bool any = false;
        for (int s = 0; s < SK_COUNT; ++s)
            if (m_has(g.party[sel], s)) { sks += kSkills[s].name; sks += "  "; any = true; }
        if (!any) sks += "none yet";
        fb_text(8, 130, sks, C_YELLOW);
        char buf[40];
        snprintf(buf, sizeof buf, "Gold %dG   Bells %d/4", g.gold, popcount_bells(g.flags));
        fb_text(8, 142, buf, C_WIN_FG);
        render_submit();
        poll_keys();
        if (key_hit(pd::key::Up)) sel = (sel + PARTY_SIZE - 1) % PARTY_SIZE;
        if (key_hit(pd::key::Down)) sel = (sel + 1) % PARTY_SIZE;
        if (key_hit(pd::key::Enter) || key_hit(pd::key::Esc)) { clear_hits(); return; }
        pd::sys::sleep_ms(15);
    }
}

// Cursor over the party (drawn as a status window on the map).
int pick_ally_map(int start, bool only_dead) {
    int t = start;
    auto ok = [&](int i) { return !only_dead || g.party[i].dead; };
    for (int k = 0; k < PARTY_SIZE && !ok(t); ++k) t = (t + 1) % PARTY_SIZE;
    clear_hits();
    for (;;) {
        draw_map(g);
        win(2, 110, 156, 50);
        for (int i = 0; i < PARTY_SIZE; ++i) {
            const Member& m = g.party[i];
            const int cx = 6, cy = 113 + i * 12;
            char line[48];
            if (m.dead)
                snprintf(line, sizeof line, "%s Lv%d DEAD", m.name, m.level);
            else
                snprintf(line, sizeof line, "%s Lv%d HP %d/%d MP %d/%d", m.name, m.level, m.hp, m_hpmax(m),
                         m.mp, m_mpmax(m));
            fb_text(cx, cy, line, i == t ? C_FLASH : (m.dead ? C_RED : C_WIN_FG));
            if (i == t) fb_text(cx - 8, cy, ">", C_YELLOW);
        }
        render_submit();
        poll_keys();
        if (key_hit(pd::key::Left) || key_hit(pd::key::Up)) { int nt = t; do { nt = (nt + 3) % PARTY_SIZE; } while (!ok(nt)); t = nt; }
        if (key_hit(pd::key::Right) || key_hit(pd::key::Down)) { int nt = t; do { nt = (nt + 1) % PARTY_SIZE; } while (!ok(nt)); t = nt; }
        if (key_hit(pd::key::Enter) || key_hit(pd::key::Space)) { clear_hits(); return t; }
        if (key_hit(pd::key::Esc)) { clear_hits(); return -1; }
        pd::sys::sleep_ms(15);
    }
}

void items_screen() {
    for (;;) {
        std::vector<std::string> rows;
        std::vector<int> ids;
        for (int it = 0; it < IT_COUNT; ++it)
            if (g.items[it] > 0) {
                char buf[40];
                snprintf(buf, sizeof buf, "%s x%d", kItems[it].name, g.items[it]);
                rows.push_back(buf);
                ids.push_back(it);
            }
        rows.push_back("Back");
        const int r = list_menu([] { draw_map(g); }, "ITEMS", rows);
        if (r < 0 || r == int(ids.size())) return;
        const int it = ids[r];
        const int t = pick_ally_map(0, kItems[it].revive);
        if (t < 0) continue;
        Member& m = g.party[t];
        const ItemDef& idf = kItems[it];
        if (idf.revive) {
            if (!m.dead) { msg("Only the fallen can be revived."); continue; }
            m.dead = 0;
            m.hp = int16_t(m_hpmax(m) * idf.power / 100);
            msg(std::string(m.name) + " is revived!");
        } else if (it == IT_POTION) {
            const int amt = std::min(m_hpmax(m) - m.hp, idf.power);
            m.hp += amt;
            msg(std::string(m.name) + " recovered " + std::to_string(amt) + " HP.");
        } else {
            const int amt = std::min(m_mpmax(m) - m.mp, idf.power);
            m.mp += amt;
            msg(std::string(m.name) + " recovered " + std::to_string(amt) + " MP.");
        }
        g.items[it]--;
    }
}

void save_menu() {
    std::vector<std::string> rows;
    for (int s = 0; s < SAVE_SLOTS; ++s) {
        char desc[64];
        if (storage_slot_info(s, desc, sizeof desc)) rows.push_back("Slot " + std::to_string(s + 1) + ": " + desc);
        else rows.push_back("Slot " + std::to_string(s + 1) + ": no SD card");
    }
    rows.push_back("Back");
    const int r = list_menu([] { draw_map(g); }, "SAVE", rows);
    if (r < 0 || r >= SAVE_SLOTS) return;
    switch (storage_save(r, g)) {
    case SAVE_OK: msg("Saved to slot " + std::to_string(r + 1) + "."); break;
    case SAVE_NO_CARD: msg("No SD card!"); break;
    default: msg("Save failed!"); break;
    }
}

void shop() {
    for (;;) {
        std::vector<std::string> rows;
        for (int it = 0; it < IT_COUNT; ++it) {
            char buf[40];
            snprintf(buf, sizeof buf, "%s  %dG", kItems[it].name, kItems[it].price);
            rows.push_back(buf);
        }
        rows.push_back("Leave");
        char title[32];
        snprintf(title, sizeof title, "SHOP - %dG", g.gold);
        const int r = list_menu([] { draw_map(g); }, title, rows);
        if (r < 0 || r == IT_COUNT) return;
        if (g.gold < kItems[r].price) { msg("Not enough gold!"); continue; }
        g.gold -= kItems[r].price;
        g.items[r]++;
        msg("Bought a " + std::string(kItems[r].name) + ".");
    }
}

void inn() {
    const int r = list_menu([] { draw_map(g); }, "INN - 10G", {"Rest", "Leave"});
    if (r != 0) return;
    if (g.gold < 10) { msg("The innkeeper shakes his head: 10G, warden."); return; }
    g.gold -= 10;
    heal_all(g);
    printf("[LB] rested at the inn\n");
    run_dialog({"The party sleeps soundly. HP and MP restored."});
}

// ---------------------------------------------------------------- map loop
enum MapExit { MAP_CONTINUE = 0, MAP_TITLE };

void game_over() {
    run_dialog({"The Mist Sea takes the last light...", "GAME OVER - the title screen awaits."});
}

void open_chest(int fx, int fy) {
    const ChestDef* c = chest_at(g.map, fx, fy);
    if (!c) return;
    if (g.chests & (1 << c->chest_index)) { msg("The chest is empty."); return; }
    g.chests |= uint8_t(1 << c->chest_index);
    g.gold += c->gold;
    std::string line = "Found " + std::to_string(c->gold) + "G";
    if (c->item >= 0) {
        g.items[c->item]++;
        line += " and a " + std::string(kItems[c->item].name);
    }
    line += "!";
    printf("[LB] chest %d opened\n", c->chest_index);
    msg(line);
}

void ring_bell() {
    const int tower = g.map - MAP_TOWER1;
    if (tower < 0 || tower > 3) return;
    if (g.flags & flag_bell(tower)) { msg("The bell hums softly."); return; }
    run_dialog(bell_pages(tower));
    g.flags |= flag_bell(tower);
    const int exp = 120 + 80 * tower, gold = 100 + 50 * tower;
    g.gold += gold;
    gain_exp(g, exp);
    printf("[LB] bell rung %d/4\n", popcount_bells(g.flags));
}

void talk(int npc_idx) {
    const NpcDef& n = npc(npc_idx);
    if (n.dialog == NPC_SHOP_Q || n.dialog == NPC_SHOP_M) { shop(); return; }
    if (n.dialog == NPC_INN_Q || n.dialog == NPC_INN_M) { inn(); return; }
    run_dialog(npc_pages(n.dialog, g));
    if (n.dialog == NPC_ELDER && !(g.flags & F_QUEST)) {
        g.flags |= F_QUEST;
        g.gold += 50;
        g.items[IT_POTION] += 2;
        printf("[LB] quest started\n");
    }
}

void interact() {
    const int fx = g.px + kDx[g.dir], fy = g.py + kDy[g.dir];
    const int ni = npc_at(g.map, fx, fy);
    if (ni >= 0) { talk(ni); return; }
    const SignDef* s = sign_at(g.map, fx, fy);
    if (s) { run_dialog(sign_pages(s->sign, g)); return; }
    const ChestDef* c = chest_at(g.map, fx, fy);
    if (c) { open_chest(fx, fy); return; }
    if (map_tile(g.map, fx, fy) == T_BELL) { ring_bell(); return; }
}

MapExit boss_check() {
    const BossDef* bd = boss_near(g.map, g.px, g.py, 2);
    if (!bd) return MAP_CONTINUE;
    if (bd->need && (g.flags & bd->need) != bd->need) return MAP_CONTINUE;
    const bool done = bd->enemy == EN_VEY
        ? (g.flags & F_VEY) != 0
        : (g.map >= MAP_TOWER1 && g.map <= MAP_TOWER4 && (g.flags & flag_guard(g.map - MAP_TOWER1)));
    if (done) return MAP_CONTINUE;
    if (bd->enemy == EN_VEY) run_dialog(vey_pages());
    EncGroup grp = {{uint8_t(bd->enemy), 0, 0, 0}, 1, 1};
    const BattleResult r = run_battle(g, grp, bd->enemy == EN_VEY);
    if (r == BR_LOSE) { game_over(); return MAP_TITLE; }
    if (r == BR_WIN && bd->enemy != EN_VEY && g.map >= MAP_TOWER1 && g.map <= MAP_TOWER4)
        g.flags |= flag_guard(g.map - MAP_TOWER1);
    if (r == BR_WIN && bd->enemy == EN_VEY) {
        g.flags |= F_VEY | F_WON;
        printf("[LB] game won\n");
        run_dialog(ending_pages());
        return MAP_TITLE;
    }
    return MAP_CONTINUE;
}

MapExit maybe_encounter() {
    const int t = map_tile(g.map, g.px, g.py);
    int table = tile_danger(t);
    if (table < 0) table = map_def(g.map).danger;
    if (table < 0) return MAP_CONTINUE;
    if (rnd(g.rng, ENCOUNTER_RATE) != 0) return MAP_CONTINUE;
    int n = 0;
    const EncGroup* groups = enc_groups(table, &n);
    int total = 0;
    for (int i = 0; i < n; ++i) total += groups[i].weight;
    int r = rnd(g.rng, total), gi = 0;
    while (gi < n - 1 && r >= groups[gi].weight) { r -= groups[gi].weight; ++gi; }
    if (run_battle(g, groups[gi], false) == BR_LOSE) {
        game_over();
        return MAP_TITLE;
    }
    return MAP_CONTINUE;
}

MapExit try_step() {
    const int nx = g.px + kDx[g.dir], ny = g.py + kDy[g.dir];
    if (tile_solid(map_tile(g.map, nx, ny)) || npc_at(g.map, nx, ny) >= 0) return MAP_CONTINUE;
    g.px = int16_t(nx);
    g.py = int16_t(ny);
    g.frame ^= 1;
    ++g.steps;
    if (const ExitDef* e = exit_at(g.map, nx, ny)) {
        g.map = uint8_t(e->to_map);
        g.px = int16_t(e->to_x);
        g.py = int16_t(e->to_y);
        printf("[LB] enter %s\n", map_def(g.map).name);
        return MAP_CONTINUE;
    }
    if (boss_check() == MAP_TITLE) return MAP_TITLE;
    return maybe_encounter();
}

bool esc_menu() {   // true = quit to title
    const int r = list_menu([] { draw_map(g); }, "MENU",
                            {"Status", "Items", "Save", "Help", "Quit to title"});
    if (r < 0) return false;
    if (r == 0) status_screen();
    else if (r == 1) items_screen();
    else if (r == 2) save_menu();
    else if (r == 3) run_dialog(help_pages());
    else if (r == 4) {
        const int q = list_menu([] { draw_map(g); }, "QUIT", {"Yes", "No"});
        if (q == 0) return true;
    }
    return false;
}

MapExit map_loop() {
    int move_dir = -1;
    uint32_t next_move = 0;
    clear_hits();
    for (;;) {
        poll_keys();
        const uint32_t now = pd::sys::now_ms();
        // A press steps at once (scripted taps arrive as press+release in one
        // poll); a held arrow repeats after MOVE_DELAY_MS.
        int nd = -1;
        bool pressed = false;
        if (key_hit(pd::key::Up)) { nd = 1; pressed = true; }
        else if (key_hit(pd::key::Down)) { nd = 0; pressed = true; }
        else if (key_hit(pd::key::Left)) { nd = 2; pressed = true; }
        else if (key_hit(pd::key::Right)) { nd = 3; pressed = true; }
        if (nd < 0) {
            if (key_down(pd::key::Up)) nd = 1;
            else if (key_down(pd::key::Down)) nd = 0;
            else if (key_down(pd::key::Left)) nd = 2;
            else if (key_down(pd::key::Right)) nd = 3;
        }
        if (nd >= 0) {
            if (pressed || nd != move_dir) {
                move_dir = nd;
                g.dir = uint8_t(nd);
                if (try_step() == MAP_TITLE) return MAP_TITLE;
                next_move = now + MOVE_DELAY_MS;
            } else if (now >= next_move) {
                if (try_step() == MAP_TITLE) return MAP_TITLE;
                next_move = now + MOVE_REPEAT_MS;
            }
        } else {
            move_dir = -1;
        }
        if (key_hit(pd::key::Enter) || key_hit(pd::key::Space)) interact();
        if (key_hit(pd::key::Esc) && esc_menu()) return MAP_TITLE;
        if (key_hit(pd::key::F1)) run_dialog(help_pages());
        g.play_ms += 10;
        draw_map(g);
        render_submit();
        pd::sys::sleep_ms(10);
    }
}

// ---------------------------------------------------------------- title
void draw_title_bg() {
    fb_clear(0);
    fb_text_big(2, 22, "THE LAST BELL", C_YELLOW);
    fb_text_center(50, "the sky of Vael is breaking", C_CYAN);
    for (int i = 0; i < 4; ++i) fb_tile(56 + i * 12, 66, T_BELL);
    fb_text_center(138, std::string("v") + PD_APP_VERSION + " - arrows+enter", C_DIM);
}

int title_screen() {   // returns 0 when a game starts
    int sel = 0;
    static const char* const items[3] = {"NEW GAME", "CONTINUE", "HELP"};
    clear_hits();
    for (;;) {
        draw_title_bg();
        for (int i = 0; i < 3; ++i) {
            const int y = 92 + i * 14;
            if (i == sel) fb_fill(52, y - 1, 60, 12, C_SEL);
            fb_text(56, y, items[i], i == sel ? C_WIN_FG : C_DIM);
        }
        render_submit();
        poll_keys();
        if (key_hit(pd::key::Up)) sel = (sel + 2) % 3;
        if (key_hit(pd::key::Down)) sel = (sel + 1) % 3;
        if (key_hit(pd::key::Enter) || key_hit(pd::key::Space)) {
            clear_hits();
            if (sel == 0) {
                new_game(g);
                run_dialog(intro_pages(), false);
                printf("[LB] intro done, entering %s\n", map_def(g.map).name);
                return 0;
            }
            if (sel == 1) {
                std::vector<std::string> rows;
                for (int s = 0; s < SAVE_SLOTS; ++s) {
                    char desc[64];
                    if (storage_slot_info(s, desc, sizeof desc))
                        rows.push_back("Slot " + std::to_string(s + 1) + ": " + desc);
                    else
                        rows.push_back("Slot " + std::to_string(s + 1) + ": no SD card");
                }
                rows.push_back("Back");
                const int r = list_menu(draw_title_bg, "CONTINUE", rows);
                if (r >= 0 && r < SAVE_SLOTS) {
                    if (storage_load(r, g) == SAVE_OK) return 0;
                    msg("That slot cannot be loaded.", false);
                }
            }
            if (sel == 2) run_dialog(help_pages(), false);
        }
        pd::sys::sleep_ms(15);
    }
}

}  // namespace

int main() {
    pd::diag::begin("The Last Bell", PD_APP_VERSION, kStages, pd::array_count(kStages));
    pd::diag::stage(kStart);
    pd::Terminal& term = pd::terminal_init();
    pd::gfx::init(term.display());
    pd::diag::display_ready();
    pd::diag::stage(kDisplay);
    keys_attach(&term.keyboard());
    art_init();
    pd::diag::stage(kArt);
    maps_init();
    pd::diag::stage(kMaps);
    pd::fs::mount();   // quiet; saves re-mount and report when there is no card
    if (!render_init()) {
        pd::gfx::clear(0x000000);
        pd::gfx::text(8, 8, 0, pd::gfx::font(pd::gfx::FONT_8x16), "The Last Bell: not enough RAM", 0xFF4040, 0x000000);
        for (;;) pd::sys::sleep_ms(1000);
    }
    pd::diag::stage(kReady);
    pd::diag::done();
    for (;;) {
        if (title_screen() == 0) map_loop();
    }
}
