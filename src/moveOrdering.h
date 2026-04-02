#pragma once

#include <vector>

#include "board.h"
#include "constants.h"
#include "evaluate.h"
#include "moveList.h"

inline int moveScore(const Board& board, const Move move) {
    int from = moveFrom(move);
    int to = moveTo(move);
    int type = moveType(move);

    int attacker = board.squares[from] & 7;
    int victim   = board.squares[to]   & 7;
    if (victim == NO_PIECE_TYPE && type != EN_PASSANT) return 0;

    int victimVal = (type == EN_PASSANT) ? 100 : PIECE_VALUES[victim];
    return victimVal * 10 - PIECE_VALUES[attacker];
}
