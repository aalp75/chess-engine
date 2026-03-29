#pragma once

#include <cstdint>

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
    Move moves[256] = {};
    int count = 0;

    void addMove(int from, int to, int type = 0, int promo = 0) {
        moves[count++] = encodeMove(from, to, type, promo);
    }
};
