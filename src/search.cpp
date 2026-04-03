#include<vector>
#include<algorithm>
#include<cstring>

#include "search.h"
#include "constants.h"
#include "moves.h"
#include "board.h"
#include "evaluate.h"
#include "moveOrdering.h"
#include "moveList.h"
#include "transpositionTable.h"
#include "timeManager.h"

/*
TODO: 
- clean the code
- early return in mate found

*/

constexpr int MAX_QDEPTH = 50;

int killers[2][256];

Move findBestMove(Board& board, int maxDepth, TimeManager& timeManager, SearchStats& stats) {

    StateInfo states[256];
    Move bestMove = 0;
    int bestScore = -INF;

    for (int depth = 1; depth <= maxDepth; depth++) {

        //memset(killers, 0, sizeof(killers));

        MoveList moves = generateMoves(board);

        Move ttMove = 0;
        TTEntry& entry = tt[board.key & (TT_SIZE - 1)];
        if (entry.key == board.key) {
            ttMove = entry.bestMove;
        }

        int currentBestScore = -INF;
        Move currentBestMove = 0;
        int ply = 0;
    
        // killer move
        for (int i = 0; i < moves.count; i++) {
            Move move = moves.moves[i];
            if (move == ttMove) {
                moves.scores[i] = 1'000'000;
            }
            else if (move == killers[0][ply]) {
                moves.scores[i] = 900'000;
            }
            else if (move == killers[1][ply]) {
                moves.scores[i] = 800'000;
            }
            else {
                moves.scores[i] = moveScore(board, move);
            }
        }
        
        for (int i = 0; i < moves.count; i++) {
            pickBest(moves, i);
            Move move = moves.moves[i];
            doMove(board, move, states, ply);
            if (!board.isInCheck(board.side ^ 1)) {
                int score = -negamax(
                    board,
                    states,
                    depth - 1,
                    -INF,
                    INF,
                    ply + 1,
                    timeManager,
                    stats
                );
                if (score > currentBestScore) {
                    // we found a new best move
                    currentBestScore = score;
                    currentBestMove = move;
                }
            }
            undoMove(board, states, ply);
        }

        // only accept a depth result if the search completed
        if (stats.stopped) break;
        
        if (currentBestMove != 0) { // if it's a valid move
            bestMove = currentBestMove;
            bestScore = currentBestScore;
            stats.depth = depth;
        }

        // checkmate found, it's not needed to search deeper
        if (bestScore > MATE_BOUND) break; 
    }

    stats.score = bestScore;
    return bestMove;
}

int negamax(Board& board, 
            StateInfo* states, 
            int depth, 
            int alpha, 
            int beta, 
            int ply, 
            TimeManager& timeManager,
            SearchStats& stats) 
{
    
    if (stats.stopped) return alpha;
    
    stats.nodes++;

    if ((stats.nodes & 4095) == 0 && timeManager.isExpired()) {
        stats.stopped = true;
        return alpha;
    }

    if (depth == 0) {
        return quiescenceSearch(board, states, alpha, beta, ply, 0, timeManager, stats);
    }

    Key key = board.key;
    int ttScore;
    if (probeTT(key, depth, ply, alpha, beta, ttScore)) {
        stats.ttHits++;
        return ttScore;
    }

    // null pruning
    if (depth >= 3 && !board.isInCheck(board.side)) {

        doNullMove(board, states, ply);
        int nullScore = -negamax(
            board, 
            states, 
            depth - 3, 
            -beta, 
            -beta + 1, 
            ply + 1, 
            timeManager, 
            stats
        );
        undoNullMove(board, states, ply);

        if (stats.stopped) {
            return alpha;
        }

        if (nullScore >= beta) {
            return beta;
        }
    }

    int originalAlpha = alpha;
    Move bestMove = 0;

    MoveList moves = generateMoves(board);
    Move ttMove = 0;
    TTEntry& entry = tt[key & (TT_SIZE - 1)];
    if (entry.key == key) {
        ttMove = entry.bestMove;
    }
    for (int i = 0; i < moves.count; i++) {
        Move move = moves.moves[i];
        if (move == ttMove) {
            moves.scores[i] = 1'000'000;
        }
        else if (move == killers[0][ply]) {
            moves.scores[i] = 900'000;
        }
        else if (move == killers[1][ply]) {
            moves.scores[i] = 800'000;
        }
        else {
            moves.scores[i] = moveScore(board, move);
        }
    }

    int legal = 0;

    for (int i = 0; i < moves.count; i++) {
        pickBest(moves, i);
        Move move = moves.moves[i];
        doMove(board, move, states, ply);
        if (board.isInCheck(board.side ^ 1)) {
            undoMove(board, states, ply);
            continue;
        }
        int score = -negamax(
            board, 
            states, 
            depth - 1, 
            -beta,
            -alpha,
            ply + 1,
            timeManager, 
            stats
        );
        undoMove(board, states, ply);   
        if (stats.stopped) return alpha;

        legal += 1;

        if (score >= beta) { // prune
            if (states[ply].capturedPiece == NO_PIECE) { // quiet move
                killers[1][ply] = killers[0][ply];
                killers[0][ply] = move;
            }
            storeTT(key, depth, ply, beta, TT_BETA, move);
            return beta;
        }
        if (score > alpha) {
            bestMove = move;
            alpha = score;
        }
    }

    if (legal == 0) { // checkmate or stalemate
        int score = board.isInCheck(board.side) ? -(INF - ply) : 0;
        storeTT(key, depth, ply, score, TT_EXACT, 0);
        return score;
    }

    TTFlag flag = TT_EXACT;
    if (alpha <= originalAlpha) {
        flag = TT_ALPHA;
    }

    storeTT(key, depth, ply, alpha, flag, bestMove);
    return alpha;
}

int quiescenceSearch(Board& board, StateInfo* states, int alpha, int beta, int ply, int qdepth, TimeManager& timeManager, SearchStats& stats) {

    if (stats.stopped) return alpha;

    if ((stats.qnodes & 4095) == 0 && timeManager.isExpired()) {
        stats.stopped = true;
        return alpha;
    }

    if (ply > stats.seldepth) {
        stats.seldepth = ply;
    }

    int eval = evaluate(board);
    if (eval >= beta) {
        return beta;
    }
    if (qdepth > MAX_QDEPTH) { // drop it as some point
        return eval;
    }

    stats.qnodes++;

    alpha = std::max(alpha, eval);

    MoveList moves = generateTacticalMoves(board);
    for (int i = 0; i < moves.count; i++) {
        moves.scores[i] = moveScore(board, moves.moves[i]);
    }

    for (int i = 0; i < moves.count; i++) {
        pickBest(moves, i);
        Move move = moves.moves[i];
        
        doMove(board, move, states, ply);
        if (!board.isInCheck(board.side ^ 1)) {
            int score = -quiescenceSearch(board, states, -beta, -alpha, ply + 1, qdepth + 1, timeManager, stats);
            if (score >= beta) { 
                undoMove(board, states, ply); 
                return beta;
            }
            alpha = std::max(score, alpha);
        }
        undoMove(board, states, ply);

        if (stats.stopped) {
            return alpha;
        }
    }
    return alpha;

}
