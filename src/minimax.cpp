#include <algorithm>

#include "minimax.h"
#include "board.h"
#include "constants.h"
#include "moves.h"
#include "evaluate.h"

Move findBestMoveMinimax(Board& board, int depth, SearchStats& stats) {

    StateInfo states[256];
    Move bestMove = 0;

    int bestScore = -INF;
    int ply = 0;

    MoveList moves = generateMoves(board);

    for (int i = 0; i < moves.count; i++) {
        Move move = moves.moves[i];
        doMove(board, move, states, ply);

        if (!board.isInCheck(board.side ^ 1)) {
            int score = minimax(board, states, depth - 1, ply + 1, 1);

            if (score > bestScore) {
                bestScore = score;
                bestMove = move;
            }
        }

        undoMove(board, states, ply);
    }

    stats.score = bestScore;
    return bestMove;
}

/** 
 * Implements minimax function
 * Player 0 tries to maximise his score while player 1 tries to minimise it
 */

int minimax(Board& board, StateInfo* states, int depth, int ply, int player) {
    if (depth == 0) {
        int eval = evaluate(board);
        return (player == 0) ? eval : -eval;
    }

    MoveList moves = generateMoves(board);

    int bestScore = (player == 0) ? -INF : INF;

    for (int i = 0; i < moves.count; i++) {
        Move move = moves.moves[i];
        doMove(board, move, states, ply);
        if (!board.isInCheck(board.side ^ 1)) {
            int score = minimax(board, states, depth - 1, ply + 1, player ^ 1);
            bestScore = (player == 0) ? std::max(bestScore, score) : std::min(bestScore, score);
        }
        undoMove(board, states, ply);
    }
    return bestScore;
}