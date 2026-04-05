#pragma once

#include <string>
#include <algorithm>

#include "constants.h"

uint64_t random64();

template <typename T>
int popcount(T t) {
    return __builtin_popcountll(static_cast<uint64_t>(t));
}

template <typename T>
int lsb(T t) {
    return __builtin_ctzll(static_cast<uint64_t>(t));
}

inline std::string formatNumber(long long nodes) {
    if (nodes >= 1000000) return std::to_string(nodes / 100000 / 10.0).substr(0, 4) + "M";
    if (nodes >= 1000)    return std::to_string(nodes / 100 / 10.0).substr(0, 4) + "K";
    return std::to_string(nodes);
}

// Chebyshev distance between 2 squares
// maximum of rank and file difference
inline int distance(int square1, int square2) {
    return std::max(std::abs(square1 / 8 - square2 / 8), std::abs((square1 % FILE_NB) - (square2 % FILE_NB)));
}