#pragma once

#include <cstdint>
#include <algorithm>

#include "constants.h"

inline Move encodeMove(int from, int to, int type, int promo) {
    return from | (to << 6) | (type << 12) | (promo << 16);
}

inline int moveFrom(Move move) {
    return move & 0x3F;
}

inline int moveTo(Move move) {
    return (move >> 6) & 0x3F;
}

inline int moveType(Move move) {
    return (move >> 12) & 0x3;
}

inline int movePromo(Move move) {
    return (move >> 16) & 0xF;
}

struct MoveList {
    Move moves[256]; // it's less than the maximum number of move possible from a position
    int scores[256]; // it's used for sorting
    int count = 0;

    void addMove(int from, int to, int type = 0, int promo = 0) {
        moves[count++] = encodeMove(from, to, type, promo);
    }
};

/**
 * pickBest does a selection sort on-the-fly. It loop over all moves and ensure
 * that the moves[startIndex] is the best move in [startIndex, ..., end]
 */
inline void pickBest(MoveList& moves, int startIndex) {
    int bestIndex = startIndex;
    for (int i = startIndex + 1; i < moves.count; i++) {
        if (moves.scores[i] > moves.scores[bestIndex]) {
            bestIndex = i;
        }
    }
    if (bestIndex != startIndex) {
        std::swap(moves.moves[startIndex], moves.moves[bestIndex]);
        std::swap(moves.scores[startIndex], moves.scores[bestIndex]);
    }
}
