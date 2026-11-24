// The Last Bell - game data and story text.
#include "data.h"

#include "art.h"

#include "game.h"

namespace lb {

const ClassDef kClasses[CLS_COUNT] = {
    // name        hp  mp atk def agi   hp_g mp_g atk_g def_g agi_g
    [CLS_WARRIOR] = {"Warrior", 38, 6,  9, 7, 6,   6, 1, 2, 1, 1},
    [CLS_WHITE]   = {"W. Mage", 24, 14, 5, 3, 7,   3, 3, 1, 1, 1},
    [CLS_BLACK]   = {"B. Mage", 22, 16, 5, 3, 8,   3, 4, 1, 1, 1},
    [CLS_THIEF]   = {"Thief",   28, 10, 7, 4, 12,  4, 2, 2, 1, 2},
};

const SkillDef kSkills[SK_COUNT] = {
    [SK_HEAL]       = {"Heal",       6,  40, TGT_ONE_ALLY,   true},
    [SK_CURA]       = {"Cura",       16, 30, TGT_ALL_ALLIES, true},
    [SK_BOLT]       = {"Bolt",       6,  26, TGT_ONE_ENEMY,  false},
    [SK_BLAZE]      = {"Blaze",      14, 30, TGT_ALL_ENEMIES,false},
    [SK_FROST]      = {"Frost",      12, 46, TGT_ONE_ENEMY,  false},
    [SK_CLEAVE]     = {"Cleave",     10, 22, TGT_ALL_ENEMIES,false},
    [SK_SECOND_WIND]= {"2nd Wind",   8,  50, TGT_ONE_ALLY,   true},  // self only
    [SK_STEAL]      = {"Steal",      6,  0,  TGT_ONE_ENEMY,  false},  // gold
    [SK_VANISH]     = {"Vanish",     8,  0,  TGT_ONE_ENEMY,  false},  // escape
    [SK_NIX]        = {"Nix",        10, 36, TGT_ONE_ENEMY,  false},
};

const ItemDef kItems[IT_COUNT] = {
    [IT_POTION]  = {"Potion",      20, 40, false},
    [IT_ETHER]   = {"Ether",       40, 12, false},
    [IT_PHOENIX] = {"Phoenix Down",150, 50, true},
};

const EnemyDef kEnemies[EN_COUNT] = {
    // name            hp atk def agi exp gold sprite        cast
    [EN_IMP]       = {"Mist Imp",      14,  6, 2,  8,  8,  6, SPR_IMP,  -1},
    [EN_WOLF]      = {"Thorn Wolf",    18,  9, 3, 12, 12,  8, SPR_WOLF, -1},
    [EN_MOTH]      = {"Grave Moth",    10,  7, 1, 14, 10, 10, SPR_MOTH, -1},
    [EN_NEWT]      = {"Bog Newt",      24,  8, 5,  5, 16, 12, SPR_NEWT, -1},
    [EN_CINDER]    = {"Cinder Sprite", 16, 12, 2, 16, 20, 15, SPR_CINDER, SK_BOLT},
    [EN_BEETLE]    = {"Iron Beetle",   30, 10,10,  4, 22, 20, SPR_BEETLE, -1},
    [EN_BAT]       = {"Night Bat",     14, 11, 2, 18, 18, 14, SPR_BAT,  -1},
    [EN_WISP]      = {"Hollow Wisp",   26, 14, 4, 10, 30, 25, SPR_WISP, SK_BOLT},
    [EN_TROLL]     = {"Mire Troll",    60, 16, 8,  3, 50, 40, SPR_TROLL, -1},
    [EN_GOLEM]     = {"Stone Golem",   80, 20,14,  2, 80, 60, SPR_GOLEM, -1},
    [EN_HOUND]     = {"Mute Hound",    70, 18, 6, 14, 90, 80, SPR_HOUND, -1},
    [EN_WURM]      = {"Silence Wurm",  110,22,10,  6, 140,120,SPR_WURM, -1},
    [EN_QGOLEM]    = {"Quiet Golem",   150,26,16,  4, 220,180,SPR_QGOLEM, -1},
    [EN_REVENANT]  = {"Dusk Revenant", 190,32,12, 10, 320,260,SPR_REVENANT, SK_FROST},
    [EN_VEY]       = {"VEY, Hollow King", 400, 40, 18, 12, 1200, 0, SPR_VEY, SK_FROST},
};

const Learn kLearn[CLS_COUNT][4] = {
    [CLS_WARRIOR] = {{3, SK_CLEAVE}, {7, SK_SECOND_WIND}, {0, 0}, {0, 0}},
    [CLS_WHITE]   = {{2, SK_HEAL}, {6, SK_CURA}, {0, 0}, {0, 0}},
    [CLS_BLACK]   = {{2, SK_BOLT}, {5, SK_BLAZE}, {9, SK_FROST}, {0, 0}},
    [CLS_THIEF]   = {{3, SK_STEAL}, {6, SK_VANISH}, {8, SK_NIX}, {0, 0}},
};
const int kLearnCount[CLS_COUNT] = {2, 2, 3, 3};

int exp_needed(int level) { return 25 * level * level; }

const char* class_name(int cls) { return kClasses[cls].name; }

// ---------------------------------------------------------------- encounters
static const EncGroup kEncMeadow[] = {
    {{EN_IMP, 0, 0, 0}, 1, 4}, {{EN_IMP, EN_IMP, 0, 0}, 2, 3},
    {{EN_IMP, EN_WOLF, 0, 0}, 2, 2}, {{EN_WOLF, EN_MOTH, 0, 0}, 2, 2},
    {{EN_MOTH, EN_MOTH, 0, 0}, 2, 1},
};
static const EncGroup kEncHollow[] = {
    {{EN_WISP, 0, 0, 0}, 1, 3}, {{EN_WISP, EN_BAT, 0, 0}, 2, 2},
    {{EN_WISP, EN_WISP, 0, 0}, 2, 2}, {{EN_TROLL, 0, 0, 0}, 1, 1},
    {{EN_CINDER, EN_MOTH, 0, 0}, 2, 1},
};
static const EncGroup kEncTower1[] = {
    {{EN_WOLF, EN_MOTH, 0, 0}, 2, 3}, {{EN_IMP, EN_IMP, EN_WOLF, 0}, 3, 2},
    {{EN_MOTH, EN_MOTH, 0, 0}, 2, 2}, {{EN_WOLF, 0, 0, 0}, 1, 2},
};
static const EncGroup kEncTower2[] = {
    {{EN_NEWT, 0, 0, 0}, 1, 3}, {{EN_BAT, EN_BAT, 0, 0}, 2, 3},
    {{EN_NEWT, EN_BAT, 0, 0}, 2, 2}, {{EN_CINDER, 0, 0, 0}, 1, 1},
};
static const EncGroup kEncTower3[] = {
    {{EN_BEETLE, 0, 0, 0}, 1, 3}, {{EN_BAT, EN_CINDER, 0, 0}, 2, 2},
    {{EN_BEETLE, EN_BAT, 0, 0}, 2, 2}, {{EN_WISP, EN_WISP, 0, 0}, 2, 1},
};
static const EncGroup kEncTower4[] = {
    {{EN_GOLEM, 0, 0, 0}, 1, 2}, {{EN_BEETLE, EN_WISP, 0, 0}, 2, 2},
    {{EN_BAT, EN_BAT, EN_CINDER, 0}, 3, 2}, {{EN_WISP, EN_WISP, EN_WISP, 0}, 3, 1},
};
static const EncGroup kEncSpire1[] = {
    {{EN_WISP, EN_BAT, 0, 0}, 2, 3}, {{EN_CINDER, EN_WISP, 0, 0}, 2, 2},
    {{EN_BAT, EN_BAT, 0, 0}, 2, 2},
};
static const EncGroup kEncSpire2[] = {
    {{EN_TROLL, 0, 0, 0}, 1, 2}, {{EN_WISP, EN_CINDER, 0, 0}, 2, 3},
    {{EN_TROLL, EN_WISP, 0, 0}, 2, 2},
};
static const EncGroup kEncSpire3[] = {
    {{EN_REVENANT, 0, 0, 0}, 1, 2}, {{EN_GOLEM, EN_WISP, 0, 0}, 2, 2},
    {{EN_CINDER, EN_CINDER, EN_WISP, 0}, 3, 1},
};
static const struct { const EncGroup* g; int n; } kEncTables[ENC_COUNT] = {
    {kEncMeadow, 5}, {kEncHollow, 5}, {kEncTower1, 4}, {kEncTower2, 4},
    {kEncTower3, 4}, {kEncTower4, 4}, {kEncSpire1, 3}, {kEncSpire2, 3},
    {kEncSpire3, 3},
};

const EncGroup* enc_groups(int table, int* count) {
    if (table < 0 || table >= ENC_COUNT) table = ENC_MEADOW;
    *count = kEncTables[table].n;
    return kEncTables[table].g;
}

// ---------------------------------------------------------------- story
static std::vector<std::string> pages(std::initializer_list<const char*> p) {
    return std::vector<std::string>(p.begin(), p.end());
}

std::vector<std::string> intro_pages() {
    return pages({
        "Long ago the sky of Vael cracked like old ice, and the world broke into a thousand drifting isles.",
        "Only the Four Bells keep them aloft. Their song holds the Mist Sea down.",
        "Now the Hollow King Vey walks the dark and silences the bells one by one. Where a bell falls silent, an isle dies.",
        "You are four Wardens of Quayhaven, the last free isle. Ring the bells. Stop the Hollow King.",
        "Your journey begins.",
    });
}

std::vector<std::string> npc_pages(int npc, const Game& g) {
    const bool quest = g.flags & F_QUEST;
    const int bells = popcount_bells(g.flags);
    switch (npc) {
    case NPC_ELDER:
        if (!quest)
            return pages({
                "Elder Maera: The bells dim, Wardens. Vey's Mutes nest in every tower.",
                "Take these supplies. Ring all four bells, then climb the Veilspire to the north-east!",
                "The isle of Quayhaven gives you 50G and 2 Potions!",
            });
        if (bells < 4) {
            char buf[96];
            snprintf(buf, sizeof buf, "Elder Maera: %d of 4 bells ring true. The wind still carries their song. Hurry!", bells);
            return pages({buf});
        }
        return pages({
            "Elder Maera: All four bells sing! Now the Veilspire, north-east. Unmake the Hollow King!",
        });
    case NPC_VILLAGER1:
        return pages({"Villager: They say the Mutes fear the sound of steel. Hit them hard and fast."});
    case NPC_VILLAGER2:
        return pages({"Villager: Tall grass hides mist imps. The shop sells Potions - buy some before the towers."});
    case NPC_CHILD:
        return pages({"Child: My granddad rang the Bell of the Gale once. He says the view is the whole sky."});
    case NPC_SHOP_Q:
        return pages({"Shopkeeper: Welcome! Potions, Ethers, Phoenix Downs. Gold talks, warden."});
    case NPC_INN_Q:
        return pages({"Innkeeper: A bed and a prayer, 10G. Sleep well, warden."});
    case NPC_VILLAGER_M1:
        return pages({"Mistral villager: The Tide bell sank the old fishery. Ring it and the sea will remember mercy."});
    case NPC_VILLAGER_M2:
        return pages({"Mistral villager: The Veilspire drinks the light around it. Stock up before you go."});
    case NPC_SHOP_M:
        return pages({"Shopkeeper: Mistral prices, quayhaven welcome. Anything catch your eye?"});
    case NPC_INN_M:
        return pages({"Innkeeper: The inn of Mistral. 10G for a full rest."});
    }
    return pages({"..."});
}

std::vector<std::string> sign_pages(int sign, const Game& g) {
    (void)g;
    switch (sign) {
    case SIGN_QUAY:     return pages({"Quayhaven - last free isle of the Drift. Mind the sky-bridges."});
    case SIGN_QUAY_WEST:return pages({"West bridge: Bell of the Ember. Its Mute is a Silence Wurm."});
    case SIGN_MISTRAL:  return pages({"Mistral - waypoint of the middle Drift."});
    case SIGN_SPIRE:    return pages({"Veilspire - seat of the Hollow King. Turn back, little warden."});
    case SIGN_TOWER_E:  return pages({"Bell of the Dawn, east isle. Ring it and the morning returns."});
    }
    return pages({"..."});
}

std::vector<std::string> bell_pages(int bell) {
    static const char* const names[4] = {"Dawn", "Tide", "Ember", "Gale"};
    static const char* const what[4] = {
        "The isle steadies in the wind.",
        "The Mist Sea answers, far below.",
        "Warm light spreads across the Drift.",
        "The sky remembers how to blow.",
    };
    char buf[96];
    snprintf(buf, sizeof buf, "You strike the Bell of the %s. Its voice rolls across the Drift, bright and clear.", names[bell]);
    char buf2[80];
    snprintf(buf2, sizeof buf2, "The Bell of the %s rings again! %s", names[bell], what[bell]);
    return pages({buf, buf2});
}

std::vector<std::string> vey_pages() {
    return pages({
        "VEY: Little wardens. You ring bells for a world that was already broken.",
        "I will keep the silence. It is kinder than your song.",
        "VEY raises the dark - the last battle begins!",
    });
}

std::vector<std::string> ending_pages() {
    return pages({
        "The Hollow King is unmade. His silence scatters like ash on the wind.",
        "Above you, all four bells ring as one.",
        "The Drift holds its breath - then the old sun climbs over the Mist Sea.",
        "Quayhaven will sing tonight. The bells will remember your names.",
        "THE END - thank you for playing The Last Bell!",
    });
}

}  // namespace lb
