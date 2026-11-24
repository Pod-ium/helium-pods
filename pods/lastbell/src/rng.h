// The Last Bell - deterministic RNG (Numerical Recipes LCG), so a new game
// and its scripted host sessions replay exactly.
#pragma once
#include <cstdint>

namespace lb {

inline uint32_t rnd_next(uint32_t& s) {
    s = s * 1664525u + 1013904223u;
    return s >> 8;
}

inline int rnd(uint32_t& s, int n) {   // 0..n-1
    return n > 0 ? int(rnd_next(s) % uint32_t(n)) : 0;
}

}  // namespace lb
