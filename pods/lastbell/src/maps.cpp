// The Last Bell - maps: the ASCII art below is decoded to tile ids at start-up
// (one char per tile, legend in tile_for_char), together with the NPCs, exits,
// chests, signs and boss triggers placed on them.
#include "maps.h"

#include <cstring>

#include "art.h"
#include "data.h"
#include "game.h"

namespace lb {

namespace {

static const char* const kMapQuay[32] = {
    "TTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTT",
    "T..............................T",
    "T..............................T",
    "T..............................T",
    "T..............................T",
    "T..............................T",
    "T...rrrrrr..rrrrrr..rrrrrr.....T",
    "T...rrrrrr..rrrrrr..rrrrrr.....T",
    "T...wowwww..wowwww..wowwww.....T",
    "T...wwwwww..wwdwww..wwdwww.....T",
    "T.....p.......p.......p........T",
    "T.....p.......p.......p........T",
    "T..pppppppppppppppppppppppppp..T",
    "T........s......p..............T",
    "T...............p.W............T",
    "T...............p..............T",
    "T..pppppppppppppppppppppppppp..T",
    "T..............................T",
    "T..............................T",
    "T..............................T",
    "T.......rrrrrr........rrrrrr...T",
    "T.......rrrrrr........rrrrrr...T",
    "T.......wowwww........wowwww...T",
    "T.......wwwwww........wwwwww...T",
    "T..............................T",
    "T..............................T",
    "T..............................T",
    "T....s.........................T",
    "T..............................T",
    "T..............................T",
    "T..............................T",
    "TTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTT",
};

static const char* const kMapMistral[32] = {
    "TTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTT",
    "T..............................T",
    "T..............................T",
    "T..............................T",
    "T..............................T",
    "T..............................T",
    "T...rrrrrrrrrrrr..rrrrrr.......T",
    "T...rrrrrrrrrrrr..rrrrrr.......T",
    "T...wowwwwwowwww..wowwww.......T",
    "T...wwwwwwwwdwww..wwdwww.......T",
    "T.....p.....p.......p..........T",
    "T.....p.....p.......p..........T",
    "T..pppppppppppppppppppppppppp..T",
    "T...............p..............T",
    "T...............p..............T",
    "T............s..p..............T",
    "T..pppppppppppppppppppppppppp..T",
    "T..............................T",
    "T.......................W......T",
    "T..............................T",
    "T.......rrrrrr........rrrrrr...T",
    "T.......rrrrrr........rrrrrr...T",
    "T.......wowwww........wowwww...T",
    "T.......wwwwww........wwwwww...T",
    "T..............................T",
    "T..............................T",
    "T..............................T",
    "T..............................T",
    "T..............................T",
    "T..............................T",
    "T..............................T",
    "TTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTT",
};

static const char* const kMapInn[16] = {
    "wwwwwwwwwwwwwwww",
    "wppppppppppppppw",
    "wppppppppppppppw",
    "wppppCCCCCCppppw",
    "wpppccccccccpppw",
    "wpppccccccccpppw",
    "wpppccccccccpppw",
    "wpppccccccccpppw",
    "wpppccccccccpppw",
    "wpppccccccccpppw",
    "wpppccccccccpppw",
    "wppbbppppppppppw",
    "wppppppppppppppw",
    "wppppppppppppppw",
    "wppppppppppppppw",
    "wwwwwwwwDwwwwwww",
};

static const char* const kMapShop[16] = {
    "wwwwwwwwwwwwwwww",
    "wpkkkkkkkkkkkkpw",
    "wpkkkkkkkkkkkkpw",
    "wppppppppppppppw",
    "wppppppppppppppw",
    "wppppppppppppppw",
    "wppppppppppppppw",
    "wpppCCCCCCCCpppw",
    "wppppppppppppppw",
    "wppppppppppppppw",
    "wppppppppppppppw",
    "wppppppppppppppw",
    "wppppppppppppppw",
    "wppppppppppppppw",
    "wppppppppppppppw",
    "wwwwwwwwDwwwwwww",
};

static const char* const kMapTower1[24] = {
    "########################",
    "#.........###..........#",
    "#.........#E#..........#",
    "#.........#.#..........#",
    "#......................#",
    "#....c.................#",
    "#......................#",
    "#......................#",
    "#.....o..........o.....#",
    "#......................#",
    "#......................#",
    "#......................#",
    "#......................#",
    "#......................#",
    "#......................#",
    "#.....o..........o.....#",
    "#......................#",
    "#......................#",
    "#......................#",
    "#......................#",
    "#......................#",
    "#......................#",
    "#...........v..........#",
    "########################",
};

static const char* const kMapTower2[24] = {
    "########################",
    "#.........###..........#",
    "#.........#E#..........#",
    "#.........#.#..........#",
    "#......................#",
    "#.................c....#",
    "#......................#",
    "#...........o..........#",
    "#......................#",
    "#......................#",
    "#......................#",
    "#......................#",
    "#....o.............o...#",
    "#......................#",
    "#......................#",
    "#......................#",
    "#......................#",
    "#...........o..........#",
    "#......................#",
    "#......................#",
    "#......................#",
    "#......................#",
    "#...........v..........#",
    "########################",
};

static const char* const kMapTower3[24] = {
    "########################",
    "#.........###..........#",
    "#.........#E#..........#",
    "#.........#.#..........#",
    "#......................#",
    "#......................#",
    "#....o............o....#",
    "#......................#",
    "#......................#",
    "#......................#",
    "#......................#",
    "#......................#",
    "#...........o..........#",
    "#......................#",
    "#......................#",
    "#......................#",
    "#......................#",
    "#......................#",
    "#....o............o....#",
    "#......................#",
    "#....c.................#",
    "#......................#",
    "#...........v..........#",
    "########################",
};

static const char* const kMapTower4[24] = {
    "########################",
    "#.........###..........#",
    "#.........#E#..........#",
    "#.........#.#..........#",
    "#......................#",
    "#...o..............o...#",
    "#......................#",
    "#......................#",
    "#......................#",
    "#......................#",
    "#...o..............o...#",
    "#......................#",
    "#...........o..........#",
    "#......................#",
    "#......................#",
    "#......................#",
    "#......................#",
    "#...o..............o...#",
    "#.................c....#",
    "#......................#",
    "#......................#",
    "#......................#",
    "#...........v..........#",
    "########################",
};

static const char* const kMapSpire1[24] = {
    "########################",
    "#......................#",
    "#...........^..........#",
    "#......................#",
    "#...c..................#",
    "#......................#",
    "#......................#",
    "#......................#",
    "#......................#",
    "#......o........o......#",
    "#......................#",
    "#......................#",
    "#......................#",
    "#......................#",
    "#......................#",
    "#......o........o......#",
    "#......................#",
    "#......................#",
    "#......................#",
    "#......................#",
    "#......................#",
    "#......................#",
    "#...........v..........#",
    "########################",
};

static const char* const kMapSpire2[24] = {
    "########################",
    "#......................#",
    "#...........^..........#",
    "#......................#",
    "#..................c...#",
    "#......................#",
    "#......................#",
    "#......................#",
    "#...........o..........#",
    "#......................#",
    "#......................#",
    "#......................#",
    "#.....o...........o....#",
    "#......................#",
    "#......................#",
    "#......................#",
    "#...........o..........#",
    "#......................#",
    "#......................#",
    "#......................#",
    "#......................#",
    "#......................#",
    "#...........v..........#",
    "########################",
};

static const char* const kMapSpire3[24] = {
    "########################",
    "#......................#",
    "#......................#",
    "#......................#",
    "#......................#",
    "#...........o..........#",
    "#......................#",
    "#......................#",
    "#...........o..........#",
    "#......................#",
    "#......................#",
    "#......................#",
    "#.....o...........o....#",
    "#......................#",
    "#......................#",
    "#......................#",
    "#...........o..........#",
    "#......................#",
    "#......................#",
    "#...c..................#",
    "#......................#",
    "#......................#",
    "#...........v..........#",
    "########################",
};

static const char* const kMapWorld[64] = {
    "~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~",
    "~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~",
    "~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~",
    "~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~sssssssssss~~",
    "~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~s.........s~~",
    "~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~s.xxxxxxx.s~~",
    "~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~s,xxxxxxx.s~~",
    "~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~sssssssssssssssss~s.xxxxxxx.s~~",
    "~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~s.,T.T..,....f.fs~s.xxxxxxx.s~~",
    "~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~s....f...,....S.s~s.xsxxxxx.s~~",
    "~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~sS.T.......N.g..s~spxxVxxxx.s~~",
    "~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~s.f...TT.,.Tg...s~s.xxpxxxx.s~~",
    "~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~sS..S...S,..S.S.s~s.xxxxxxx.s~~",
    "~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~s.......f.fSS..Ss~s.xxxxxxx.s~~",
    "~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~s.,fS.Spp4.....fBBB.xxxxxxx.s~~",
    "~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~s.ST.TSp....,..Ss~s.xxxxxxx.s~~",
    "~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~s.S....N.SNT..f.s~s.........s~~",
    "~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~s...f..p...S.S..s~sssssssssss~~",
    "~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~s.S....p..g..g..s~~~~~~~~~~~~~~",
    "~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~s.....fS.....S.Ss~~~~~~~~~~~~~~",
    "~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~s......p........s~~~~~~~~~~~~~~",
    "~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~sssssssBsssssssss~~~~~~~~~~~~~~",
    "~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~B~~~~~~~~~~~~~~~~~~~~~~~",
    "~~~sssssssssssssssss~~~~~~~~~~~~~~~~~~~~B~~~~~~~~~~~~~~~~~~~~~~~",
    "~~~s.,...f.......f.s~~~~~~~~~~~~~~~~~~~~B~~~~~~~~~~~~~~~~~~~~~~~",
    "~~~s.f.N..........fs~~~~~~~sssssssssssssBsssss~~~~~~~~~~~~~~~~~~",
    "~~~s....N.T........s~~~~~~~s......N.T..,p..M.s~~~~~~~~~~~~~~~~~~",
    "~~~s.,....f....,...s~~~~~~~s......f.....p...Ms~~~~~~~~~~~~~~~~~~",
    "~~~s..T.T.,........s~~~~~~~s..NTTTT,,...p..M.s~~~~~~~~~~~~~~~~~~",
    "~~~s..f.....M..,...s~~~~~~~s.....g.,....pM...s~sssssssssssssss~~",
    "~~~s....T.p3.......s~~~~~~~s...f....N...p...fs~s.,...........s~~",
    "~~~s.T..T.p..f..M.,s~~~~~~~s..g,...N.g.Mp....s~sfN.TfT.......s~~",
    "~~~s.....,p....M...s~~~~~~~s...T...NRpppp.f..s~s.........,...s~~",
    "~~~s..,...p........s~~~~~~~s.f....g..pf....,.s~s...f.NNg.,...s~~",
    "~~~s......p.f......s~~~~~~~s.......T.p.......s~sf...N........s~~",
    "~~~s......p....M...s~~~~~~~s.......f.p,......s~s....,..g,....s~~",
    "~~~sf.....p,...,,..s~~~~~~~s.......f.p.......s~sfT....fT.....s~~",
    "~~~sssssssBsssssssssssssssss........gp.......s~s....,s...g..,s~~",
    "~~~s.,....,....f..........,s.........Tf.f....BBBpppppp1......s~~",
    "~~~s.,..f............TT....s.........T.,.....s~s......f......s~~",
    "~~~s.......................s.........NT.N.f..s~s.............s~~",
    "~~~s.............g.....T...B......,..p.T.....s~s.........M...s~~",
    "~~~sf.,.ppppppppppppTTpppppB.........pT......s~s...f.........s~~",
    "~~~s....p.....f............s.........p..N....s~sf............s~~",
    "~~~s....Q..,.......Tf......s.........p.......s~s....f.fM.f...s~~",
    "~~~s.............T,..N.....ssssssssssBssssssss~s......,M..M..s~~",
    "~~~s.f...N.T.......g,.g....s~~~~~~~~~B~~~~~~~~~s.............s~~",
    "~~~s..T.,N,................s~ssssssssBssssssssssssssssssssssss~~",
    "~~~s.......NT.T........,...s~s.......p.........s~~~~~~~~~~~~~~~~",
    "~~~s..TT...g.........f.....s~s.......p.........s~~~~~~~~~~~~~~~~",
    "~~~s...f...g.....,.........s~s.......p.....f...s~~~~~~~~~~~~~~~~",
    "~~~s.............,.f,.,....s~s.......p.,...f...s~~~~~~~~~~~~~~~~",
    "~~~s........Tg.g..f........s~s.......p....TN...s~~~~~~~~~~~~~~~~",
    "~~~s.......N...g.......f...s~s....,..p..T....f.s~~~~~~~~~~~~~~~~",
    "~~~s...T..T..N.......f.....s~sf......p,gg.....,s~~~~~~~~~~~~~~~~",
    "~~~s.....,.................s~s.......p2f.T..,..s~~~~~~~~~~~~~~~~",
    "~~~s........g,.......f.....s~s..,f....Tg,......s~~~~~~~~~~~~~~~~",
    "~~~s.......................s~s.......T.gT.T.f..s~~~~~~~~~~~~~~~~",
    "~~~s.....,.f.f.............s~s.................s~~~~~~~~~~~~~~~~",
    "~~~sssssssssssssssssssssssss~s............f....s~~~~~~~~~~~~~~~~",
    "~~~~~~~~~~~~~~~~~~~~~~~~~~~~~s............f....s~~~~~~~~~~~~~~~~",
    "~~~~~~~~~~~~~~~~~~~~~~~~~~~~~sssssssssssssssssss~~~~~~~~~~~~~~~~",
    "~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~",
    "~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~",
};

const MapDef kMaps[MAP_COUNT] = {
    [MAP_WORLD]       = {"The Drift",        64, 64, kMapWorld,   -1},
    [MAP_QUAY]        = {"Quayhaven",        32, 32, kMapQuay,    -1},
    [MAP_QUAY_INN]    = {"Quayhaven inn",    16, 16, kMapInn,     -1},
    [MAP_QUAY_SHOP]   = {"Quayhaven shop",   16, 16, kMapShop,    -1},
    [MAP_MISTRAL]     = {"Mistral",          32, 32, kMapMistral, -1},
    [MAP_MISTRAL_INN] = {"Mistral inn",      16, 16, kMapInn,     -1},
    [MAP_MISTRAL_SHOP]= {"Mistral shop",     16, 16, kMapShop,    -1},
    [MAP_TOWER1]      = {"Bell of the Dawn",  24, 24, kMapTower1, ENC_TOWER1},
    [MAP_TOWER2]      = {"Bell of the Tide",  24, 24, kMapTower2, ENC_TOWER2},
    [MAP_TOWER3]      = {"Bell of the Ember", 24, 24, kMapTower3, ENC_TOWER3},
    [MAP_TOWER4]      = {"Bell of the Gale",  24, 24, kMapTower4, ENC_TOWER4},
    [MAP_SPIRE1]      = {"Veilspire I",       24, 24, kMapSpire1, ENC_SPIRE1},
    [MAP_SPIRE2]      = {"Veilspire II",      24, 24, kMapSpire2, ENC_SPIRE2},
    [MAP_SPIRE3]      = {"Veilspire III",     24, 24, kMapSpire3, ENC_SPIRE3},
};

const NpcDef kNpcs[] = {
    {MAP_QUAY, 6, 13, SPR_ELDER, NPC_ELDER},
    {MAP_QUAY, 18, 13, SPR_VILLAGER, NPC_VILLAGER1},
    {MAP_QUAY, 24, 12, SPR_VILLAGER, NPC_VILLAGER2},
    {MAP_QUAY, 11, 17, SPR_CHILD, NPC_CHILD},
    {MAP_QUAY_INN, 7, 2, SPR_INN, NPC_INN_Q},
    {MAP_QUAY_SHOP, 8, 6, SPR_SHOP, NPC_SHOP_Q},
    {MAP_MISTRAL, 10, 13, SPR_VILLAGER, NPC_VILLAGER_M1},
    {MAP_MISTRAL, 22, 13, SPR_VILLAGER, NPC_VILLAGER_M2},
    {MAP_MISTRAL_INN, 7, 2, SPR_INN, NPC_INN_M},
    {MAP_MISTRAL_SHOP, 8, 6, SPR_SHOP, NPC_SHOP_M},
};

const ExitDef kExits[] = {
    {MAP_WORLD, 8, 44, MAP_QUAY, 8, 14},
    {MAP_QUAY, 16, 30, MAP_WORLD, 8, 45},
    {MAP_QUAY, 14, 9, MAP_QUAY_INN, 8, 14},
    {MAP_QUAY_INN, 8, 15, MAP_QUAY, 14, 10},
    {MAP_QUAY, 22, 9, MAP_QUAY_SHOP, 8, 14},
    {MAP_QUAY_SHOP, 8, 15, MAP_QUAY, 22, 10},
    {MAP_WORLD, 36, 32, MAP_MISTRAL, 16, 14},
    {MAP_MISTRAL, 16, 30, MAP_WORLD, 36, 33},
    {MAP_MISTRAL, 12, 9, MAP_MISTRAL_INN, 8, 14},
    {MAP_MISTRAL_INN, 8, 15, MAP_MISTRAL, 12, 10},
    {MAP_MISTRAL, 20, 9, MAP_MISTRAL_SHOP, 8, 14},
    {MAP_MISTRAL_SHOP, 8, 15, MAP_MISTRAL, 20, 10},
    {MAP_WORLD, 54, 38, MAP_TOWER1, 12, 21},
    {MAP_TOWER1, 12, 22, MAP_WORLD, 54, 39},
    {MAP_WORLD, 38, 55, MAP_TOWER2, 12, 21},
    {MAP_TOWER2, 12, 22, MAP_WORLD, 38, 56},
    {MAP_WORLD, 11, 30, MAP_TOWER3, 12, 21},
    {MAP_TOWER3, 12, 22, MAP_WORLD, 11, 31},
    {MAP_WORLD, 42, 14, MAP_TOWER4, 12, 21},
    {MAP_TOWER4, 12, 22, MAP_WORLD, 42, 15},
    {MAP_WORLD, 55, 10, MAP_SPIRE1, 12, 21},
    {MAP_SPIRE1, 12, 22, MAP_WORLD, 55, 11},
    {MAP_SPIRE1, 12, 2, MAP_SPIRE2, 12, 21},
    {MAP_SPIRE2, 12, 22, MAP_SPIRE1, 12, 3},
    {MAP_SPIRE2, 12, 2, MAP_SPIRE3, 12, 21},
    {MAP_SPIRE3, 12, 22, MAP_SPIRE2, 12, 3},
};

const ChestDef kChests[] = {
    {MAP_TOWER1, 5, 5, 0, 60, IT_POTION},
    {MAP_TOWER2, 18, 5, 1, 80, IT_ETHER},
    {MAP_TOWER3, 5, 20, 2, 100, IT_POTION},
    {MAP_TOWER4, 18, 18, 3, 150, IT_PHOENIX},
    {MAP_SPIRE1, 4, 4, 4, 120, IT_ETHER},
    {MAP_SPIRE2, 19, 4, 5, 200, IT_PHOENIX},
    {MAP_SPIRE3, 4, 19, 6, 300, IT_PHOENIX},
};

const SignDef kSigns[] = {
    {MAP_QUAY, 9, 13, SIGN_QUAY},
    {MAP_QUAY, 5, 27, SIGN_QUAY_WEST},
    {MAP_MISTRAL, 13, 15, SIGN_MISTRAL},
    {MAP_WORLD, 53, 37, SIGN_TOWER_E},
    {MAP_WORLD, 54, 9, SIGN_SPIRE},
};

const BossDef kBosses[] = {
    {MAP_TOWER1, 11, 2, EN_HOUND, 2, 0},
    {MAP_TOWER2, 11, 2, EN_WURM, 2, 0},
    {MAP_TOWER3, 11, 2, EN_QGOLEM, 2, 0},
    {MAP_TOWER4, 11, 2, EN_REVENANT, 2, 0},
    {MAP_SPIRE3, 12, 4, EN_VEY, 2, F_BELL0 | F_BELL1 | F_BELL2 | F_BELL3},
};

uint8_t g_map_tiles[MAP_COUNT][64 * 64];

}  // namespace

void maps_init() {
    for (int m = 0; m < MAP_COUNT; ++m) {
        const MapDef& d = kMaps[m];
        const bool dungeon = m >= MAP_TOWER1;
        for (int y = 0; y < d.h; ++y)
            for (int x = 0; x < d.w; ++x)
                g_map_tiles[m][y * 64 + x] = uint8_t(tile_for_char(d.rows[y][x], dungeon));
    }
}

const MapDef& map_def(int map) {
    if (map < 0 || map >= MAP_COUNT) map = MAP_WORLD;
    return kMaps[map];
}

int map_tile(int map, int x, int y) {
    const MapDef& d = map_def(map);
    if (x < 0 || y < 0 || x >= d.w || y >= d.h) return T_VOID;
    return g_map_tiles[map][y * 64 + x];
}

int tile_for_char(char c, bool dungeon) {
    if (dungeon) {
        switch (c) {
        case '.': return T_DFLOOR;
        case ',': return T_CARPET;
        case 'o': return T_PILLAR;
        default: break;
        }
    }
    switch (c) {
    case '~': return T_WATER;
    case '.': return T_GRASS;
    case ',': return T_GRASS2;
    case 'f': return T_FLOWER;
    case 'g': return T_TALL;
    case 'T': return T_TREE;
    case 'N': return T_PINE;
    case 'M': return T_MOUNTAIN;
    case 'S': return T_SNOW;
    case 's': return T_SAND;
    case 'p': return T_PATH;
    case 'B': return T_BRIDGE;
    case 'Q': case 'R': case '1': case '2': case '3': case '4': case 'V':
    case 'd': case 'D': return T_DOOR;
    case 'x': return T_CORRUPT;
    case 'w': return T_WALL;
    case 'r': return T_ROOF;
    case 'o': return T_WINDOW;
    case 'F': return T_FENCE;
    case 'W': return T_WELL;
    case '#': return T_DWALL;
    case 'E': return T_BELL;
    case 'c': return T_CHEST;
    case 'C': return T_COUNTER;
    case 'b': return T_BED;
    case 'k': return T_SHELF;
    case '^': return T_STAIRUP;
    case 'v': return T_STAIRDN;
    default: return T_VOID;
    }
}

int tile_for_char(char c) { return tile_for_char(c, false); }

bool tile_solid(int t) {
    switch (t) {
    case T_VOID: case T_TREE: case T_PINE: case T_WATER: case T_MOUNTAIN:
    case T_WALL: case T_ROOF: case T_WINDOW: case T_SIGN: case T_FENCE:
    case T_WELL: case T_DWALL: case T_PILLAR: case T_BELL: case T_CHEST:
    case T_COUNTER: case T_BED: case T_SHELF:
        return true;
    default:
        return false;
    }
}

int tile_danger(int t) {
    switch (t) {
    case T_TALL: return ENC_MEADOW;
    case T_CORRUPT: return ENC_HOLLOW;
    default: return -1;
    }
}

int npc_count() { return int(sizeof kNpcs / sizeof kNpcs[0]); }
const NpcDef& npc(int i) { return kNpcs[i]; }
int npc_at(int map, int x, int y) {
    for (int i = 0; i < npc_count(); ++i)
        if (kNpcs[i].map == map && kNpcs[i].x == x && kNpcs[i].y == y) return i;
    return -1;
}
int exit_count() { return int(sizeof kExits / sizeof kExits[0]); }
const ExitDef& exit(int i) { return kExits[i]; }
const ExitDef* exit_at(int map, int x, int y) {
    for (int i = 0; i < exit_count(); ++i)
        if (kExits[i].map == map && kExits[i].x == x && kExits[i].y == y) return &kExits[i];
    return nullptr;
}
int chest_count() { return int(sizeof kChests / sizeof kChests[0]); }
const ChestDef& chest(int i) { return kChests[i]; }
const ChestDef* chest_at(int map, int x, int y) {
    for (int i = 0; i < chest_count(); ++i)
        if (kChests[i].map == map && kChests[i].x == x && kChests[i].y == y) return &kChests[i];
    return nullptr;
}
int sign_count() { return int(sizeof kSigns / sizeof kSigns[0]); }
const SignDef& sign(int i) { return kSigns[i]; }
const SignDef* sign_at(int map, int x, int y) {
    for (int i = 0; i < sign_count(); ++i)
        if (kSigns[i].map == map && kSigns[i].x == x && kSigns[i].y == y) return &kSigns[i];
    return nullptr;
}
int boss_count() { return int(sizeof kBosses / sizeof kBosses[0]); }
const BossDef& boss(int i) { return kBosses[i]; }
const BossDef* boss_near(int map, int x, int y, int radius) {
    for (int i = 0; i < boss_count(); ++i) {
        const BossDef& b = kBosses[i];
        if (b.map != map) continue;
        const int dx = x - b.x, dy = y - b.y;
        if (dx * dx + dy * dy <= radius * radius) return &b;
    }
    return nullptr;
}

void bell_pos(int map, int* x, int* y) {
    *x = *y = -1;
    const MapDef& d = map_def(map);
    for (int yy = 0; yy < d.h; ++yy)
        for (int xx = 0; xx < d.w; ++xx)
            if (g_map_tiles[map][yy * 64 + xx] == T_BELL) { *x = xx; *y = yy; return; }
}

}  // namespace lb
