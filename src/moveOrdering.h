#pragma once

#include <vector>

#include "board.h"
#include "constants.h"
#include "evaluate.h"
#include "moveList.h"
#include "pieceSquareTable.h"

inline void scoreMoves(const Board& board, MoveList& moves, Move ttMove, int ply,
                       const int killers[2][256],
                       const int history[2][64][64],
                       const int captureHistory[7][64][7],
                       const int countermove[2][64][64],
                       Move prevMove = 0) {
    int prevFrom = moveFrom(prevMove);
    int prevTo   = moveTo(prevMove);

    for (int i = 0; i < moves.count; i++) {
        Move move = moves.moves[i];
        int  type = moveType(move);
        int  to   = moveTo(move);
        int  from = moveFrom(move);

        if (move == ttMove) {
            moves.scores[i] = 3'000'000;
            continue;
        }

        int victim = (type == EN_PASSANT) ? PAWN
                   : (type == CASTLING)   ? NO_PIECE_TYPE
                   : (board.squares[to] & 7);

        if (victim != NO_PIECE_TYPE || type == PROMOTION) {
            int attacker = board.squares[from] & 7;
            if (type == PROMOTION) {
                moves.scores[i] = 1'500'000;
            } 
            else {
                int mvvlva = PIECE_VALUES_MG[victim] * 10 - PIECE_VALUES_MG[attacker];
                moves.scores[i] = 1'000'000 + mvvlva + captureHistory[attacker][to][victim];
            }
        } 
        else if (move == Move(killers[0][ply])) {
            moves.scores[i] = 900'000;
        } 
        else if (move == Move(killers[1][ply])) {
            moves.scores[i] = 800'000;
        } 
        else if (prevMove != 0 && move == Move(countermove[board.side][prevFrom][prevTo])) {
            moves.scores[i] = 700'000;
        } 
        else {
            moves.scores[i] = history[board.side][from][to];
        }
    }
}
