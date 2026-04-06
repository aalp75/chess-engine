#include <algorithm>

#include "minimax.h"
#include "board.h"
#include "constants.h"
#include "moves.h"
#include "evaluate.h"

Move findBestMoveMinimax(Board& board, int depth, SearchStats& stats) {

    Move bestMove = 0;

    int bestScore = -INF;
    int ply = 0;

    MoveList moves = generateMoves(board);

    for (int i = 0; i < moves.count; i++) {
        Move move = moves.moves[i];
        doMove(board, move);

        if (!board.isInCheck(board.side ^ 1)) {
            int score = minimax(board, depth - 1, 1);

            if (score > bestScore) {
                bestScore = score;
                bestMove = move;
            }
        }

        undoMove(board);
    }

    stats.score = bestScore;
    return bestMove;
}

/** 
 * Implements minimax function
 * Player 0 tries to maximise his score while player 1 tries to minimise it
 */

int minimax(Board& board, int depth, int player) {
    if (depth == 0) {
        int eval = eval::evaluate(board);
        return (player == 0) ? eval : -eval;
    }

    MoveList moves = generateMoves(board);

    int bestScore = (player == 0) ? -INF : INF;

    int legal = 0;

    for (int i = 0; i < moves.count; i++) {
        Move move = moves.moves[i];
        doMove(board, move);
        if (!board.isInCheck(board.side ^ 1)) {
            legal++;
            int score = minimax(board, depth - 1, player ^ 1);
            bestScore = (player == 0) ? std::max(bestScore, score) : std::min(bestScore, score);
        }
        undoMove(board);
    }

    if (legal == 0) {
        if (board.isInCheck(board.side)) return (player == 0) ? -(MATE_SCORE - board.ply) : (MATE_SCORE - board.ply);
        return 0; // stalemate
    }
    return bestScore;
}