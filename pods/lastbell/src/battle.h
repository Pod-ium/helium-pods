// The Last Bell - turn-based battles (Final Fantasy style): random encounters
// on dangerous ground, boss fights at the bells and in the Veilspire.
#pragma once
#include "data.h"

namespace lb {

struct Game;

enum BattleResult { BR_WIN = 0, BR_LOSE, BR_FLEE };

// Runs one battle to its end (blocking, own input loop). grp: 1..4 enemy ids.
// no_flee: the enemy cannot be escaped from (the Hollow King).
BattleResult run_battle(Game& g, const EncGroup& grp, bool no_flee);

}  // namespace lb
