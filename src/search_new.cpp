#include <vector>
#include <algorithm>
#include <cstring>
#include <iostream>

#include "search.h"
#include "constants.h"
#include "moves.h"
#include "board.h"
#include "evaluate.h"
#include "moveOrdering.h"
#include "moveList.h"
#include "transpositionTable.h"
#include "timeManager.h"

/** 
 * Search the best move based on alpha-beta algorithm
 */

Move findBestMove(Board& board, int maxDepth, TimeManager& timeManager, SearchStats& stats) {

    StateInfo states[256];
    Move bestMove = 0;
    int bestScore;

    int ply = 0;

    for (int depth = 1; depth <= maxDepth; depth++) {

        int alpha = -INF;
        int beta = INF;

        bestScore = -INF;

        MoveList moves = generateMoves(board);

        for (int i = 0; i < moves.count; i++) {
            Move move = moves.moves[i];
            doMove(board, move, states, ply);

            if (!board.isInCheck(board.side ^ 1)) {
                int score = -negamax(board, states, depth - 1, -beta, -alpha, ply + 1, timeManager, stats);
                
                if (score > bestScore) {
                    bestScore = score;
                    bestMove = move;
                }
                alpha = std::max(alpha, score);
            }
            undoMove(board, states, ply);

            if (alpha >= beta) {
                break;
            }
        }
    }
    stats.score = bestScore;
    return bestMove;
}

int negamax(Board& board, StateInfo* states, int depth, int alpha, int beta, 
            int ply, TimeManager& timeManager, SearchStats& stats) {

    if (depth == 0) {
        #ifdef TEST
        return evaluate(board);
        #else
        return quiescenceSearch(board, states, alpha, beta, ply, 0, timeManager, stats);
        #endif

    }

    int alphaOrig = alpha;

    TTEntry& entry = tt[board.key & (TT_SIZE - 1)];

    MoveList moves = generateMoves(board);

    int bestScore = -INF;

    for (int i = 0; i < moves.count; i++) {
        Move move = moves.moves[i];
        doMove(board, move, states, ply);

        if (!board.isInCheck(board.side ^ 1)) {
            int score = -negamax(board, states, depth - 1, -beta, -alpha, ply + 1, timeManager, stats);
            alpha = std::max(alpha, score);
            if (score > bestScore) {
                bestScore = score;
            }
        }
        undoMove(board, states, ply);
        
        if (alpha >= beta) {
            break;
        }
    }

    return bestScore;
}

int quiescenceSearch(Board& board, StateInfo* states, int alpha, int beta, int ply, 
    int qdepth, TimeManager& timeManager, SearchStats& stats) {
    
    MoveList moves = generateTacticalMoves(board);




    return 0;
}
        

