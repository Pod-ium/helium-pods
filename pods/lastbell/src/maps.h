// The Last Bell - maps: ASCII source decoded to tile ids, plus NPCs, exits,
// chests, signs and boss triggers placed on them.
#pragma once
#include <cstdint>

namespace lb {

enum MapId {
    MAP_WORLD = 0,
    MAP_QUAY, MAP_QUAY_INN, MAP_QUAY_SHOP,
    MAP_MISTRAL, MAP_MISTRAL_INN, MAP_MISTRAL_SHOP,
    MAP_TOWER1, MAP_TOWER2, MAP_TOWER3, MAP_TOWER4,
    MAP_SPIRE1, MAP_SPIRE2, MAP_SPIRE3,
    MAP_COUNT
};

struct MapDef {
    const char* name;
    int w, h;
    const char* const* rows;
    int danger;        // default encounter table for walkable tiles, -1 = none
};

struct NpcDef { int map, x, y, sprite, dialog; };
struct ExitDef { int map, x, y, to_map, to_x, to_y; };
struct ChestDef { int map, x, y, chest_index, gold, item; };  // item: ItemId or -1
struct SignDef { int map, x, y, sign; };
struct BossDef { int map, x, y, enemy, radius; uint32_t need; };  // step within radius -> battle; need: flags required first

void maps_init();                     // decodes all maps to tile ids
const MapDef& map_def(int map);
int map_tile(int map, int x, int y);  // T_VOID outside
int tile_for_char(char c);
int tile_for_char(char c, bool dungeon);
bool tile_solid(int t);
int tile_danger(int t);               // encounter table for the tile, -1 = none

int npc_count();
const NpcDef& npc(int i);
int npc_at(int map, int x, int y);    // npc index or -1
int exit_count();
const ExitDef& exit(int i);
const ExitDef* exit_at(int map, int x, int y);
int chest_count();
const ChestDef& chest(int i);
const ChestDef* chest_at(int map, int x, int y);
int sign_count();
const SignDef& sign(int i);
const SignDef* sign_at(int map, int x, int y);
int boss_count();
const BossDef& boss(int i);
const BossDef* boss_near(int map, int x, int y, int radius);  // nullptr when its `need` flags are unmet is the caller's job
// Position of the bell tile on a tower map, or (-1,-1).
void bell_pos(int map, int* x, int* y);

}  // namespace lb
