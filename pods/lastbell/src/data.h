// The Last Bell - game data: classes, skills, items, enemies, encounter
// tables, and the story/dialogue text.
#pragma once
#include <cstdint>
#include <string>
#include <vector>

namespace lb {

struct Game;

enum ClassId { CLS_WARRIOR = 0, CLS_WHITE, CLS_BLACK, CLS_THIEF, CLS_COUNT };
enum SkillId {
    SK_HEAL = 0, SK_CURA, SK_BOLT, SK_BLAZE, SK_FROST,
    SK_CLEAVE, SK_SECOND_WIND, SK_STEAL, SK_VANISH, SK_NIX, SK_COUNT
};
enum ItemId { IT_POTION = 0, IT_ETHER, IT_PHOENIX, IT_COUNT };
enum EnemyId {
    EN_IMP = 0, EN_WOLF, EN_MOTH, EN_NEWT, EN_CINDER, EN_BEETLE, EN_BAT,
    EN_WISP, EN_TROLL, EN_GOLEM,
    EN_HOUND, EN_WURM, EN_QGOLEM, EN_REVENANT, EN_VEY, EN_COUNT
};
// Skill target kinds.
enum SkillTarget { TGT_ONE_ENEMY = 0, TGT_ALL_ENEMIES, TGT_ONE_ALLY, TGT_ALL_ALLIES };

struct ClassDef {
    const char* name;
    int hp, mp, atk, def, agi;          // at level 1
    int hp_g, mp_g, atk_g, def_g, agi_g; // growth per level
};
struct SkillDef {
    const char* name;
    int mp, power;
    int target;      // SkillTarget
    bool heal;       // power = HP restored
};
struct EnemyDef {
    const char* name;
    int hp, atk, def, agi, exp, gold;
    int sprite;      // SpriteId
    int cast;        // skill it may cast (SK_*), or -1
};
struct ItemDef {
    const char* name;
    int price, power;
    bool revive;     // power = HP restored on revive
};
struct Learn { int level, skill; };

extern const ClassDef kClasses[CLS_COUNT];
extern const SkillDef kSkills[SK_COUNT];
extern const EnemyDef kEnemies[EN_COUNT];
extern const ItemDef kItems[IT_COUNT];
extern const Learn kLearn[CLS_COUNT][4];
extern const int kLearnCount[CLS_COUNT];

int exp_needed(int level);          // EXP to go from level to level+1
const char* class_name(int cls);

// ---------------------------------------------------------------- encounters
enum EncId {
    ENC_MEADOW = 0, ENC_HOLLOW, ENC_TOWER1, ENC_TOWER2, ENC_TOWER3, ENC_TOWER4,
    ENC_SPIRE1, ENC_SPIRE2, ENC_SPIRE3, ENC_COUNT
};
struct EncGroup { uint8_t enemies[4]; uint8_t count; uint8_t weight; };
// group of enemy ids (count 1..4) and its pick weight
const EncGroup* enc_groups(int table, int* count);

// ---------------------------------------------------------------- story text
// Dialogue pages; each page is one screen of up to 3 lines.
std::vector<std::string> intro_pages();
std::vector<std::string> npc_pages(int npc, const Game& g);
std::vector<std::string> sign_pages(int sign, const Game& g);
std::vector<std::string> bell_pages(int bell);       // bell 0..3
std::vector<std::string> vey_pages();
std::vector<std::string> ending_pages();

// NPC and sign ids (used by maps.cpp).
enum NpcId {
    NPC_ELDER = 0, NPC_VILLAGER1, NPC_VILLAGER2, NPC_CHILD,
    NPC_SHOP_Q, NPC_INN_Q, NPC_VILLAGER_M1, NPC_VILLAGER_M2, NPC_SHOP_M, NPC_INN_M,
    NPC_COUNT
};
enum SignId {
    SIGN_QUAY = 0, SIGN_QUAY_WEST, SIGN_MISTRAL, SIGN_SPIRE, SIGN_TOWER_E,
    SIGN_COUNT
};

}  // namespace lb
