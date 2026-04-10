#pragma once

#include "constants.h"

namespace magic {

    void initialize();
    Bitboard getPawnAttacks(int square, int color);
    Bitboard getKnightAttacks(int square);
    Bitboard getBishopAttacks(int square, Bitboard occupied);
    Bitboard getRookAttacks(int square, Bitboard occupied);
    Bitboard getQueenAttacks(int square, Bitboard occupied);
    Bitboard getKingAttacks(int square);
    
}