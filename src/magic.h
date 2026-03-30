#pragma once

#include "constants.h"

namespace magic {

    void initMagicTables();
    Bitboard getRookAttacks(int square, Bitboard occupied);
    
}