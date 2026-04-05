#pragma once

#include <vector>

#include "board.h"
#include "constants.h"
#include "evaluate.h"
#include "moveList.h"
#include "pieceSquareTable.h"

inline int moveScore(const Board& board, const Move move) {
    int from = moveFrom(move);
    int to   = moveTo(move);
    int type = moveType(move);

    int attacker = board.squares[from] & 7;
    int victim   = board.squares[to]   & 7;
    
    if (victim == NO_PIECE_TYPE && type != EN_PASSANT) return 0;

    int victimVal = (type == EN_PASSANT) ? 100 : PIECE_VALUES_MG[victim];
    return victimVal * 10 - PIECE_VALUES_MG[attacker];
}

inline void scoreMoves(const Board& board, MoveList& moves, Move ttMove, int ply, const int (*killers)[256]) {
    for (int i = 0; i < moves.count; i++) {
        if (moves.moves[i] == ttMove) {
            moves.scores[i] = INF;
        }
        else if (moves.moves[i] == killers[0][ply]) {
            moves.scores[i] = KILLER_SCORE1;
        }
        else if (moves.moves[i] == killers[1][ply]) {
            moves.scores[i] = KILLER_SCORE2;
        }
        else {
            moves.scores[i] = moveScore(board, moves.moves[i]);
        }
    }
}
