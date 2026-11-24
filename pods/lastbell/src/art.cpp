// The Last Bell - pixel art, decoded from ASCII at start-up.
// One char = one pixel; kLegend maps char -> palette index (and colour).
// ' ' (index 0) is transparent in sprites and black in tiles.
#include "art.h"

#include <cstring>

namespace lb {

pd::Color g_palette[256];
uint8_t g_tiles[T_COUNT][64];

namespace {

struct PalEntry { char ch; pd::Color rgb; };
// Art chars occupy indices 0..26; 240..254 are the UI colours (art.h).
const PalEntry kLegend[] = {
    {' ', 0x000000}, {'k', 0x14141C}, {'w', 0xF0F0F0}, {'a', 0x9AA0B0},
    {'A', 0x5A606E}, {'r', 0xC83028}, {'R', 0x701818}, {'o', 0xE07828},
    {'y', 0xE8C838}, {'g', 0x3FA048}, {'G', 0x1E5C28}, {'b', 0x3860D8},
    {'B', 0x1C2C70}, {'n', 0x0C1430}, {'c', 0x48C8D8}, {'s', 0xE8B080},
    {'m', 0x885028}, {'M', 0x583418}, {'p', 0xB070D8}, {'P', 0x682898},
    {'e', 0xD8D8E0}, {'d', 0x8A8A98}, {'D', 0x505058}, {'t', 0xC8A028},
    {'h', 0xF8E8C8}, {'q', 0xD8C088}, {'i', 0xE8F0F8},
};
constexpr int kLegendCount = int(sizeof kLegend / sizeof kLegend[0]);

int legend_index(char c) {
    if (c == '.') return 0;   // '.' is transparent in sprites; tiles avoid it
    for (int i = 0; i < kLegendCount; ++i)
        if (kLegend[i].ch == c) return i;
    return 1;   // unknown char -> near-black, never 0 (0 is transparent)
}

// ---------------------------------------------------------------- tiles
const char* const kTileArt[T_COUNT][8] = {
    [T_VOID] = {
        "        ", "        ", "        ", "        ",
        "        ", "        ", "        ", "        ",
    },
    [T_GRASS] = {
        "gGGggGGg", "GgGGGgGg", "gGggGGGG", "GgGGGgGg",
        "GgGgGGgG", "gGGGgGGg", "GgGgGGGG", "GgGGgGgG",
    },
    [T_GRASS2] = {
        "GGgGGgGG", "gGGgGGGg", "GgGGgGGg", "ggGGGgGG",
        "GGgGGgGG", "gGGgGGgG", "GGgGGgGG", "gGGGGGgG",
    },
    [T_FLOWER] = {
        "gGGygGGg", "GgGGGgGw", "gGggGGGG", "GgGGGgGy",
        "GgGwGGgG", "gGGGgGGg", "GgGgGGGG", "GyGGgGgG",
    },
    [T_TALL] = {
        "GgGgGgGg", "gGgGgGgG", "GgGgGgGg", "gGgGgGgG",
        "GgGgGgGg", "gGgGgGgG", "GgGgGgGg", "GGGGGGGG",
    },
    [T_TREE] = {
        "ggGGGGgg", "gGgGGgGg", "GgGGGGgG", "GgGGGGgG",
        "gGgGGgGg", "ggMMMMgg", "gggmmggg", "gggmmggg",
    },
    [T_PINE] = {
        "ggggGggg", "gggGgGgg", "gggGGGgg", "ggGgGGGg",
        "ggGGGGGg", "gGgGGGGG", "gggmmggg", "gggmmggg",
    },
    [T_WATER] = {
        "bBbbbbBb", "bbBbbBbb", "bBbBbbbB", "bbbbbBbb",
        "bBbbbbBb", "bbBbbBbb", "bBbBbbbB", "bbbbbBbb",
    },
    [T_SAND] = {
        "qqqqqqqq", "qqqhqqqq", "qqqqqqqh", "qqhqqqqq",
        "qqqqqqqq", "qqqqhqqq", "qqqqqqqq", "hqqqqqqq",
    },
    [T_PATH] = {
        "qqqqqqqq", "qqMqqqqq", "qqqqqqMq", "qMqqqqqq",
        "qqqqqqqq", "qqqqMqqq", "qMqqqqqq", "qqqqqqqq",
    },
    [T_BRIDGE] = {
        "MmmmmmmM", "MmmmmmmM", "MMMMMMMM", "MmmmmmmM",
        "MmmmmmmM", "MMMMMMMM", "MmmmmmmM", "MmmmmmmM",
    },
    [T_MOUNTAIN] = {
        "gggiiggg", "ggiddggg", "giddddgg", "gddDddgg",
        "ddddDddg", "dddDdddd", "ddDddddd", "ddddDddd",
    },
    [T_SNOW] = {
        "iiiiiiii", "iiwiiiii", "iiiiiwii", "iiiiiiii",
        "iwiiiiwi", "iiiiiiii", "iiiiwiii", "iiiiiiii",
    },
    [T_WALL] = {
        "hhhhhhhh", "hdhhhhhd", "hhhhhhhh", "hhhdhhhd",
        "hhhhhhhh", "hdhhhhhd", "hhhhhhhh", "dddddddd",
    },
    [T_ROOF] = {
        "rrrrrrrr", "rRrrrrRr", "rrrrrrrr", "rRrrrrRr",
        "rrrrrrrr", "rRrrrrRr", "rrrrrrrr", "RRRRRRRR",
    },
    [T_DOOR] = {
        "MMMMMMMM", "MmmmmmmM", "MmmmmmmM", "MmmtmmmM",
        "MmmmmmmM", "MmmmmmmM", "MmmmmmmM", "MMMMMMMM",
    },
    [T_WINDOW] = {
        "hhhhhhhh", "hdhhhhhd", "hhbbbbhh", "hbwwbwbh",
        "hhbbbbhh", "hdhhhhhd", "hhhhhhhh", "dddddddd",
    },
    [T_SIGN] = {
        "MMMMMMMM", "MhhhhhhM", "MhhhhhhM", "MhhhhhhM",
        "MMMMMMMM", "gggmmggg", "gggmmggg", "gggmmggg",
    },
    [T_FENCE] = {
        "gGgGGgGg", "gGgGGgGg", "gGgGGgGg", "MmmmmmmM",
        "gGgGGgGg", "MmmmmmmM", "gGgGGgGg", "GgGgGgGg",
    },
    [T_WELL] = {
        "ggrrrrgg", "grMMMMrg", "ggmMMmgg", "gddddddg",
        "ddBdddBd", "dddddddd", "dDddddDd", "gddddddg",
    },
    [T_DFLOOR] = {
        "DDDDDDDD", "DDdDDDDD", "DDDDDDdD", "DdDDDDDD",
        "DDDDDDDD", "DDDDdDDD", "DdDDDDDD", "DDDDDDDD",
    },
    [T_DWALL] = {
        "DDDDDDDD", "DkDDDDkD", "DDDDDDDD", "DDkDDDDk",
        "DDDDDDDD", "kDDDDDDk", "DDDDDDDD", "DDkDDDDk",
    },
    [T_PILLAR] = {
        "DDDDDDDD", "DddddddD", "DddddddD", "DddddddD",
        "DddddddD", "DddddddD", "DddddddD", "DDDDDDDD",
    },
    [T_STAIRUP] = {
        "eeeeeeee", "eeeeeeee", "DDeeeeDD", "DDeeeeDD",
        "DDddddDD", "DDddddDD", "DDDDDDDD", "DDDDDDDD",
    },
    [T_STAIRDN] = {
        "kkkkkkkk", "kkkkkkkk", "DDkkkkDD", "DDkkkkDD",
        "DDddddDD", "DDddddDD", "DDDDDDDD", "DDDDDDDD",
    },
    [T_BELL] = {
        "DDDttDDD", "DDttttDD", "DDttttDD", "DttttttD",
        "DttttttD", "DDtwwtDD", "DDddddDD", "DddddddD",
    },
    [T_CHEST] = {
        "DDDDDDDD", "DMMMMMMD", "DmmmmmmD", "DmmtmmmD",
        "DmmmmmmD", "DMMMMMMD", "DDDDDDDD", "DDDDDDDD",
    },
    [T_CARPET] = {
        "pppppppp", "pPPPPPPp", "pPpPPpPp", "pPPPPPPp",
        "pPpPPpPp", "pPPPPPPp", "pPPPPPPp", "pppppppp",
    },
    [T_COUNTER] = {
        "mmmmmmmm", "mMMMMMMm", "MmmmmmmM", "MmmmmmmM",
        "MmmmmmmM", "MmmmmmmM", "MmmmmmmM", "MMMMMMMM",
    },
    [T_BED] = {
        "DDDDDDDD", "DhhbbbbD", "DhhbbbbD", "DbbbbbbD",
        "DbbbbbbD", "DbBbbbbD", "DbbbbbbD", "DDDDDDDD",
    },
    [T_SHELF] = {
        "MMMMMMMM", "MrbgypcM", "MMmmmmMM", "McbgyrpM",
        "MMmmmmMM", "MyrgbpcM", "MmmmmmmM", "MMMMMMMM",
    },
    [T_CORRUPT] = {
        "PpPpPPPp", "PpPPPpPp", "pPpPPpPP", "PpPPPpPP",
        "PpPpPPpP", "pPPPpPPp", "PpPpPPPP", "PpPPpPpP",
    },
};

// ---------------------------------------------------------------- sprites
// 16 rows of 16 chars each. Player sprites: down frame 0/1 and left frame
// 0/1 are authored; up = down with the face replaced by the back of the
// head, right = mirror of left.
const char* const kSprWarriorDown0[16] = {
    "....aaaaaaaa....", "..aarrrrrrraa...", "..arrrrrrrraa...", "..assssssssa....",
    "..asksssksa.....", "..assssssssa....", "...rrrrrrrr.....", "..arrrrrrrra....",
    "..asrrrrrrsa....", "..assssssssa....", "..krrrrrrrrk....", "..krrrrrrrrk....",
    "..kkk....kkk....", "...kk....kk.....", "................", "................",
};
const char* const kSprWarriorDown1[16] = {
    "....aaaaaaaa....", "..aarrrrrrraa...", "..arrrrrrrraa...", "..assssssssa....",
    "..asksssksa.....", "..assssssssa....", "...rrrrrrrr.....", "..arrrrrrrra....",
    "..asrrrrrrsa....", "..assssssssa....", "..krrk..krrk....", "..krrk..krrk....",
    "..kkk....kkk....", "..kkk....kkk....", "................", "................",
};
const char* const kSprWarriorLeft0[16] = {
    "....aaaaaaaa....", "...arrrrrrra....", "..arrrrrrrra....", "..asssssssa.....",
    "..askssssa......", "..assssssa......", "...rrrrrrr......", "..arrrrrrrra....",
    "..arrrrrrrsa....", "..assssssa......", "...rrrrrrr......", "...rrrrrrr......",
    "...kk..kkk......", "...kk...kk......", "................", "................",
};
const char* const kSprWarriorLeft1[16] = {
    "....aaaaaaaa....", "...arrrrrrra....", "..arrrrrrrra....", "..asssssssa.....",
    "..askssssa......", "..assssssa......", "...rrrrrrr......", "..arrrrrrrra....",
    "..arrrrrrrsa....", "..assssssa......", "...rrrrrrr......", "..rrrrr.rr......",
    "...kk...kkk.....", "...kk....kk.....", "................", "................",
};

const char* const kSprWhiteDown0[16] = {
    ".....wwww.......", "...wwhhhhww.....", "..whsssssshw....", "..whsssssshw....",
    "..whsksskshw....", "..whsssssshw....", "...wwwwww.......", "..wwwwwwww......",
    "..wcwwwwwwcw....", "..wcwwwwwwcw....", "...wwwwww.......", "...wwwwww.......",
    "...ww..ww.......", "...kk..kk.......", "................", "................",
};
const char* const kSprWhiteDown1[16] = {
    ".....wwww.......", "...wwhhhhww.....", "..whsssssshw....", "..whsssssshw....",
    "..whsksskshw....", "..whsssssshw....", "...wwwwww.......", "..wwwwwwww......",
    "..wcwwwwwwcw....", "..wcwwwwwwcw....", "...wwwwww.......", "..wwwwwwww......",
    "..ww....ww......", "..kk....kk......", "................", "................",
};
const char* const kSprWhiteLeft0[16] = {
    ".....wwww.......", "...wwhhhhw......", "..whssssshw.....", "..whssssshw.....",
    "..whskssshw.....", "..whssssshw.....", "...wwwwww.......", "..wwwwwwww......",
    "..wwcwwwww......", "..wwcwwwww......", "...wwwwww.......", "...wwwwww.......",
    "...ww..ww.......", "...kk...kk......", "................", "................",
};
const char* const kSprWhiteLeft1[16] = {
    ".....wwww.......", "...wwhhhhw......", "..whssssshw.....", "..whssssshw.....",
    "..whskssshw.....", "..whssssshw.....", "...wwwwww.......", "..wwwwwwww......",
    "..wwcwwwww......", "..wwcwwwww......", "...wwwwww.......", "..wwww.ww.......",
    "...ww...ww......", "...kk....kk.....", "................", "................",
};

const char* const kSprBlackDown0[16] = {
    ".....kkkk.......", "...kkPPkk.......", "..kPssssssPk....", "..kPssssssPk....",
    "..kPskssksPk....", "..kPssssssPk....", "...kpppppk......", "..kpppppppk.....",
    "..kpkkkkkkpk....", "..kpkkkkkkpk....", "...pppppp.......", "...pppppp.......",
    "...pp..pp.......", "...kk..kk.......", "................", "................",
};
const char* const kSprBlackDown1[16] = {
    ".....kkkk.......", "...kkPPkk.......", "..kPssssssPk....", "..kPssssssPk....",
    "..kPskssksPk....", "..kPssssssPk....", "...kpppppk......", "..kpppppppk.....",
    "..kpkkkkkkpk....", "..kpkkkkkkpk....", "...pppppp.......", "..pppppppp......",
    "..pp....pp......", "..kk....kk......", "................", "................",
};
const char* const kSprBlackLeft0[16] = {
    ".....kkkk.......", "...kkPPk........", "..kPssssssPk....", "..kPssssssPk....",
    "..kPskssssPk....", "..kPssssssPk....", "...kpppppk......", "..kpppppppk.....",
    "..kpppppppk.....", "..kppppppp......", "...pppppp.......", "...pppppp.......",
    "...pp..pp.......", "...kk...kk......", "................", "................",
};
const char* const kSprBlackLeft1[16] = {
    ".....kkkk.......", "...kkPPk........", "..kPssssssPk....", "..kPssssssPk....",
    "..kPskssssPk....", "..kPssssssPk....", "...kpppppk......", "..kpppppppk.....",
    "..kpppppppk.....", "..kppppppp......", "...pppppp.......", "..pppp.pp.......",
    "...pp...pp......", "...kk....kk.....", "................", "................",
};

const char* const kSprThiefDown0[16] = {
    ".....GGGG.......", "...GGggggG......", "..GgsGGGgsG.....", "..GsssssssG.....",
    "..GskssksG......", "..GsssssssG.....", "...GmmmmmG......", "..GmmmmmmmG.....",
    "..GmGmmmGmG.....", "..GsmGGmmsG.....", "...mmmmmm.......", "...mmmmmm.......",
    "...mm..mm.......", "...kk..kk.......", "................", "................",
};
const char* const kSprThiefDown1[16] = {
    ".....GGGG.......", "...GGggggG......", "..GgsGGGgsG.....", "..GsssssssG.....",
    "..GskssksG......", "..GsssssssG.....", "...GmmmmmG......", "..GmmmmmmmG.....",
    "..GmGmmmGmG.....", "..GsmGGmmsG.....", "...mmmmmm.......", "..mmmmmmmm......",
    "..mm....mm......", "..kk....kk......", "................", "................",
};
const char* const kSprThiefLeft0[16] = {
    ".....GGGG.......", "...GGgggg.......", "..GgsGGGgs......", "..GsssssssG.....",
    "..GskssssG......", "..GssssssG......", "...Gmmmmm.......", "..GmmmmmmmG.....",
    "..GmmmmmmmG.....", "..Gmmmmmm.......", "...mmmmmm.......", "...mmmmmm.......",
    "...mm..mm.......", "...kk...kk......", "................", "................",
};
const char* const kSprThiefLeft1[16] = {
    ".....GGGG.......", "...GGgggg.......", "..GgsGGGgs......", "..GsssssssG.....",
    "..GskssssG......", "..GssssssG......", "...Gmmmmm.......", "..GmmmmmmmG.....",
    "..GmmmmmmmG.....", "..Gmmmmmm.......", "...mmmmmm.......", "..mmmm.mm.......",
    "...mm...mm......", "...kk....kk.....", "................", "................",
};

// NPCs (single frame, facing down).
const char* const kSprElder[16] = {
    ".....aaaa.......", "...awwwwa.......", "..awssssswa.....", "..awskskswa.....",
    "..awssssswa.....", "..awwwwwwa......", "...wwwwww.......", "..bbwwwwwwbb....",
    "..bwwwwwwwb.....", "..bwwwwwwwb.....", "...wwwwww.......", "...wwwwww.......",
    "...ww..ww.......", "...kk..kk.......", "................", "................",
};
const char* const kSprShop[16] = {
    ".....ssss.......", "...ssssss.......", "..ssssssss......", "..skssskss......",
    "..ssssssss......", "...ssssss.......", "..mmwwwwmm......", "..mmwwwwmm......",
    "..mwwwwwwm......", "..mwwwwwwm......", "...mmmmmm.......", "...mmmmmm.......",
    "...mm..mm.......", "...kk..kk.......", "................", "................",
};
const char* const kSprInn[16] = {
    ".....ssss.......", "...ssssss.......", "..ssssssss......", "..skssskss......",
    "..ssssssss......", "...ssssss.......", "..rrwwwwrr......", "..rrwwwwrr......",
    "..rwwwwwwr......", "..rwwwwwwr......", "...mmmmmm.......", "...mmmmmm.......",
    "...mm..mm.......", "...kk..kk.......", "................", "................",
};
const char* const kSprVillager[16] = {
    ".....hhhh.......", "...hhhhhh.......", "..hssssssh......", "..hskssksh......",
    "..hssssssh......", "...ssssss.......", "..bbbbbbb.......", "..bbbbbbb.......",
    "..bbbbbbb.......", "..bsssssb.......", "...bbbbb........", "...bbbbb........",
    "...bb.bb........", "...kk.kk........", "................", "................",
};
const char* const kSprChild[16] = {
    ".....yyy........", "...yyyyyy.......", "..yssssssy......", "..yskskssy......",
    "..yssssssy......", "...ssssss.......", "..ggggggg.......", "...ggggg........",
    "...ggggg........", "...gsssg........", "...ggggg........", "................",
    "...gg.gg........", "...kk.kk........", "................", "................",
};

// Enemies (single frame, facing the party = down).
const char* const kSprImp[16] = {
    "................", "................", "....pppppp......", "..ppPPPPpp......",
    ".pPPppppPPp.....", ".pPwwppwwPp.....", ".pPpkppkPp......", "..pPppppPp......",
    "..ppPPPPpp......", "...pppppp.......", "...pp..pp.......", "..pp....pp......",
    "..pp....pp......", "................", "................", "................",
};
const char* const kSprWolf[16] = {
    "................", "..a....a........", ".aa....aa.......", ".aaa..aaaa......",
    ".awwaaaaaa......", "aaaaaaaaaa......", "akcaaaaaaa......", "aaaaaaaaaaa.....",
    ".aaaaaaaaaa.....", ".aawwaaaaaa.....", ".aa.aaaa.aa.....", ".a..aaaa..a.....",
    ".a..a..a..a.....", ".A..A..A..A.....", "................", "................",
};
const char* const kSprMoth[16] = {
    "..dd......dd....", ".dpdd....ddp....", ".dppd....dpp....", "ddPPd....dPPdd..",
    "dddPd....dPddd..", "ddddMddddMdddd..", "..ddMMMMMMdd....", "...dMMMMMMd.....",
    "....MMMMMM......", "....mMMMMm......", "...mmMMMMmm.....", "..mmmMMMMmmm....",
    "..mm..MM..mm....", "..m...MM...m....", "................", "................",
};
const char* const kSprNewt[16] = {
    "................", "................", "...gggggg.......", "..ggkyykgg......",
    ".ggggkyyggg.....", ".ggyyyyyygg.....", ".gggyyyyggg.....", ".ggyyyyyygg.....",
    "..gggggggg......", ".gggggggggg.....", ".gggg..gggg.....", ".ggg....ggg.....",
    ".gg......gg.....", ".G........G.....", "................", "................",
};
const char* const kSprCinder[16] = {
    "................", "....yyyy........", "...yooyy........", "..yooooy........",
    ".yooryooy.......", ".yooooooyy......", ".yoorrooyy......", ".yooooooyy......",
    "..yoooooy.......", "..yooooy........", "..oy..oy........", "..oy..oy........",
    ".oy....oy.......", ".yy....yy.......", "................", "................",
};
const char* const kSprBeetle[16] = {
    "................", "................", "....AAAAAA......", "..AAaaaaAA......",
    ".AAaaaaaaAA.....", ".AaaaaaaaaAA....", "AAddaaaaadAA....", "ADDaaddddAAD....",
    "ADddddddddDA....", "ADaaaaaaaaDA....", ".AaaaaaaaaA.....", "..aaaaaaaa......",
    "..kk.kk.kk......", "..k..k..k.......", "................", "................",
};
const char* const kSprBat[16] = {
    "kB............Bk", "kBB..........BBk", "kBbk........kBBk", "kBbBk..kk..kBbkB",
    ".kBbBkPPPpkBbbk.", "..kBkkPPPkkBk...", "...kkkAAAkkk....", "....kAAAAk......",
    "....karrk.......", "....kaaak.......", "...kkAAAAkk.....", "..kkk...kkk.....",
    "..kk.....kk.....", "................", "................", "................",
};
const char* const kSprWisp[16] = {
    "................", ".....cc.........", "....cccc........", "...cwwwwc.......",
    "..cwwwwwwc......", "..cwkwwkwc......", "..cwwwwwwc......", "...cwwwwc.......",
    "....cccc........", "...cc..cc.......", "..cc....cc......", "..cc....cc......",
    ".cc......cc.....", ".BB......BB.....", "................", "................",
};
const char* const kSprTroll[16] = {
    "................", "...GG....GG.....", "..GGGGGGGG......", ".GggkyyggG......",
    ".GgggyyyygG.....", ".GggyyyyygG.....", "..GggggggG......", ".GGGGGGGGGG.....",
    ".GggGggGggG.....", ".GggggggggG.....", "..gggggggg......", "..gggggggg......",
    "..gg..gg........", ".GG....GG.......", "................", "................",
};
const char* const kSprGolem[16] = {
    "................", ".....dddd.......", "....dddddd......", "...ddyddydd.....",
    "...dddddddd.....", "..dddddddddd....", ".dddddddddddd...", ".ddDddddddDdd...",
    ".dddddddddddd...", ".dddddddddddd...", "..dddddddddd....", "..dddd..dddd....",
    "..ddd....ddd....", "..DDD....DDD....", "................", "................",
};
const char* const kSprHound[16] = {
    "................", "..A......A......", ".AAp....pAA.....", ".AAAp..pAAA.....",
    ".AwwAAAAAA......", "AAAAAAAAAA......", "Akkaaaaaaa......", "AAAAAAAAAAA.....",
    ".AAAAAAAAAA.....", ".AAwwAAAAAA.....", ".AAAaaaaAAA.....", ".AA.AAA.AA......",
    ".A..A..A..A.....", ".p..p..p..p.....", "................", "................",
};
const char* const kSprWurm[16] = {
    ".....pp.........", "....pPPp........", "...pPwykp.......", "...pPPPPp.......",
    "..PPPPPP........", "..pPPpPP........", "..PPpPP.........", ".pPPPPp.........",
    ".pPpPPp.........", ".PPpPP..........", ".pPPp...........", "PPpP............",
    "PpP.............", "pP..............", "................", "................",
};
const char* const kSprQGolem[16] = {
    "................", ".....PP.......", "....PPPP........", "...PPkkPP.......",
    "...PPPPPP.......", "..PPPPPPPP......", ".PPPPkkPPPP.....", ".PPpPPPPpPP.....",
    ".PPPPPPPPPP.....", ".PPPPPPPPPP.....", "..PPPPPPPP......", "..PPP..PPP......",
    "..PPP....PPP....", "..PPP....PPP....", "................", "................",
};
const char* const kSprRevenant[16] = {
    ".....BBBB.......", "...BBBBBB.......", "..BBkkkkBB......", "..BkckckkB......",
    "..BBkkkkBB......", "...BBBBBB.......", "..BBBBBBBB......", ".BBBBBBBBBB.....",
    ".BPPBBBBPPB.....", ".BBBBBBBBBB.....", "..BBBBBBBB......", "..BBBBBBBB......",
    "..BB....BB......", "..BB....BB......", "................", "................",
};
const char* const kSprVey[16] = {
    "....tt..tt......", "...tttttttt.....", "..tPPPPPPPt.....", "..PkckckcPk.....",
    "..kPPPPPPPk.....", "..kPPPPPPk......", ".RRPPPPPPRR.....", ".RRPPPPPPRR.....",
    ".RRRPPRRRPR.....", ".RRRRRRRRRR.....", "..RRR..RRR......", "..RR....RR......",
    "..RR....RR......", "..kk....kk......", "................", "................",
};

struct SprSrc { const char* const* rows; };
const SprSrc kSprSrc[SPR_COUNT] = {
    // warrior: down0, down1, left0, left1, then derived up0/up1/right0/right1
    {kSprWarriorDown0}, {kSprWarriorDown1}, {kSprWarriorLeft0}, {kSprWarriorLeft1},
    {nullptr}, {nullptr}, {nullptr}, {nullptr},
    // white mage
    {kSprWhiteDown0}, {kSprWhiteDown1}, {kSprWhiteLeft0}, {kSprWhiteLeft1},
    {nullptr}, {nullptr}, {nullptr}, {nullptr},
    // black mage
    {kSprBlackDown0}, {kSprBlackDown1}, {kSprBlackLeft0}, {kSprBlackLeft1},
    {nullptr}, {nullptr}, {nullptr}, {nullptr},
    // thief
    {kSprThiefDown0}, {kSprThiefDown1}, {kSprThiefLeft0}, {kSprThiefLeft1},
    {nullptr}, {nullptr}, {nullptr}, {nullptr},
    // NPCs
    {kSprElder}, {kSprShop}, {kSprInn}, {kSprVillager}, {kSprChild},
    {nullptr}, {nullptr}, {nullptr},
    // enemies 40..54
    {kSprImp}, {kSprWolf}, {kSprMoth}, {kSprNewt}, {kSprCinder},
    {kSprBeetle}, {kSprBat}, {kSprWisp}, {kSprTroll}, {kSprGolem},
    {kSprHound}, {kSprWurm}, {kSprQGolem}, {kSprRevenant}, {kSprVey},
};

// Back-of-head colour per class, used to derive the up sprites.
const char kBackColor[4] = {'a', 'w', 'k', 'G'};

uint8_t g_spr_px[SPR_COUNT][256];
SpriteArt g_spr[SPR_COUNT];

void decode(int id, const char* const* rows, int w, int h) {
    for (int y = 0; y < h; ++y)
        for (int x = 0; x < w; ++x)
            g_spr_px[id][y * w + x] = uint8_t(legend_index(rows[y][x]));
    g_spr[id] = {uint8_t(w), uint8_t(h), g_spr_px[id]};
}

void derive_up(int dst, int src_down, int cls) {
    // copy the down sprite, replace the face rows (3..5) with the back colour
    for (int y = 0; y < 16; ++y)
        for (int x = 0; x < 16; ++x) {
            uint8_t p = g_spr_px[src_down][y * 16 + x];
            if (y >= 3 && y <= 5 && p != 0) p = uint8_t(legend_index(kBackColor[cls]));
            g_spr_px[dst][y * 16 + x] = p;
        }
    g_spr[dst] = {16, 16, g_spr_px[dst]};
}

void derive_right(int dst, int src_left) {
    for (int y = 0; y < 16; ++y)
        for (int x = 0; x < 16; ++x)
            g_spr_px[dst][y * 16 + x] = g_spr_px[src_left][y * 16 + (15 - x)];
    g_spr[dst] = {16, 16, g_spr_px[dst]};
}

}  // namespace

void art_init() {
    memset(g_palette, 0, sizeof g_palette);
    for (int i = 0; i < kLegendCount; ++i) g_palette[i] = kLegend[i].rgb;
    g_palette[C_WIN_BG] = 0x101C3C;
    g_palette[C_WIN_FG] = 0xF0F0F0;
    g_palette[C_YELLOW] = 0xF0D040;
    g_palette[C_RED] = 0xE04838;
    g_palette[C_GREEN] = 0x50D858;
    g_palette[C_CYAN] = 0x50C8E0;
    g_palette[C_SEL] = 0x3050A0;
    g_palette[C_DIM] = 0x909098;
    g_palette[C_FLASH] = 0xFFFFFF;
    g_palette[C_SHADOW] = 0x080C18;

    for (int t = 0; t < T_COUNT; ++t)
        for (int y = 0; y < 8; ++y)
            for (int x = 0; x < 8; ++x)
                g_tiles[t][y * 8 + x] = uint8_t(legend_index(kTileArt[t][y][x]));

    for (int id = 0; id < SPR_COUNT; ++id)
        if (kSprSrc[id].rows) decode(id, kSprSrc[id].rows, 16, 16);
    for (int cls = 0; cls < 4; ++cls) {
        const int base = cls * 8;
        derive_up(base + 2, base + 0, cls);        // up, frame 0
        derive_up(base + 3, base + 1, cls);        // up, frame 1
        derive_right(base + 6, base + 4);          // right, frame 0
        derive_right(base + 7, base + 5);          // right, frame 1
    }
}

const SpriteArt& sprite(int id) {
    if (id < 0 || id >= SPR_COUNT) id = SPR_IMP;
    return g_spr[id];
}

}  // namespace lb
