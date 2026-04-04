#include "search.h"

#include <algorithm>
#include <iostream>
#include <cstring>

#include "board.h"
#include "constants.h"
#include "evaluate.h"
#include "moveList.h"
#include "moveOrdering.h"
#include "moves.h"
#include "timeManager.h"
#include "transpositionTable.h"

/** 
 * Search the best move based on alpha-beta algorithm
 */

/**
 * killer move for move ordering. It prioritise move with a large beta cutoff
 */

int killers[2][256];

/**
 * check game history to avoid threefold repetition draw
 */

Key gameHistory[1024];
int gameHistoryLen = 0;

inline Move getTTMove(Key key) {
    TTEntry& entry = tt[key & (TT_SIZE - 1)];
    return (entry.key == key) ? entry.bestMove : 0;
}

/**
 * It's ok to not count the legal move in findBestMove because if we go into this function
 * it means the game is not over so there is an existing move
 */
Move findBestMove(
    Board& board, 
    int maxDepth, 
    TimeManager& timeManager, 
    SearchStats& stats, 
    bool useQSearch) 
{

    std::memset(killers, 0, sizeof(killers)); // clear killers table
    
    StateInfo states[256];

    int ply = 0;
    int bestScore = -INF;
    Move bestMove = 0;

    for (int depth = 1; depth <= maxDepth; depth++) {

        Move currentBestMove = 0;
        int currentBestScore = -INF;
        int alpha = -INF;
        int beta = INF;

        // ensure that ttMove which is the best move found so far is searched first
        MoveList moves = generateMoves(board);
        Move ttMove = getTTMove(board.key);
        scoreMoves(board, moves, ttMove, ply, killers);

        for (int i = 0; i < moves.count; i++) {
            pickBest(moves, i);
            Move move = moves.moves[i];
            doMove(board, move, states, ply);

            if (!board.isInCheck(board.side ^ 1)) {
                int score = -negamax(board, states, depth - 1, -beta, -alpha, ply + 1, timeManager, stats, useQSearch);

                if (score > currentBestScore) {
                    currentBestScore = score;
                    currentBestMove = move;
                }
                alpha = std::max(alpha, score);
            }
            undoMove(board, states, ply);
        }

        // only accept a depth result if the search completed
        if (stats.stopped) break;

        if (currentBestMove != 0) {
            bestMove = currentBestMove;
            bestScore = currentBestScore;
        }

        stats.depth = depth;
        stats.score = bestScore;

        // checkmate has been found, it's not needed to search deeper
        if (bestScore > MATE_BOUND) break;

    }

    return bestMove;
}

/**
 * Implementation based on https://en.wikipedia.org/wiki/Negamax
 */
int negamax(
    Board& board, 
    StateInfo* states, 
    int depth, 
    int alpha, 
    int beta, 
    int ply, 
    TimeManager& timeManager, 
    SearchStats& stats, 
    bool useQSearch,
    bool nullMove) // avoid recursive call on nullMove 
{

    if (stats.stopped) return alpha;

    for (int i = gameHistoryLen - 3; i >= 0; i -= 2) {
        if (gameHistory[i] == board.key) return 0; // draw score
    }
    
    if (depth == 0) {
        if (useQSearch) {
            return quiescenceSearch(board, states, alpha, beta, ply, timeManager, stats);
        } 
        else {
            return evaluate(board);
        }
    }

    stats.nodes++;
    if ((stats.nodes & 4095) == 0 && timeManager.isExpired()) {
        stats.stopped = true;
        return alpha;
    }

    int alphaOrig = alpha;
    Move bestMove = 0;

    int ttScore;
    if (probeTT(board.key, depth, ply, alpha, beta, ttScore)) {
        stats.ttHits++;
        return ttScore;
    }

    /**
     * null move pruning
     * 
     * so we pass the turn and if we got a better score than the current beta we can prune
     * the branch
     */
    if (useQSearch && depth >= 3 && !nullMove && !board.isInCheck(board.side)) {
        doNullMove(board, states, ply);
        int nullScore = -negamax(board, states, depth - 3, -beta, -beta + 1, ply + 1, timeManager, stats, useQSearch, true);
        undoNullMove(board, states, ply);
        if (stats.stopped) return alpha;
        if (nullScore >= beta) return beta;
    }

    MoveList moves = generateMoves(board);
    Move ttMove = getTTMove(board.key);
    scoreMoves(board, moves, ttMove, ply, killers);

    int bestScore = -INF;

    int legalmoves = 0;

    for (int i = 0; i < moves.count; i++) {
        pickBest(moves, i);
        Move move = moves.moves[i];
        doMove(board, move, states, ply);

        if (!board.isInCheck(board.side ^ 1)) {
            legalmoves++;
            gameHistory[gameHistoryLen++] = board.key;
            int score = -negamax(board, states, depth - 1, -beta, -alpha, ply + 1, timeManager, stats, useQSearch);
            if (score > bestScore) {
                bestScore = score;
                bestMove = move;
            }
            gameHistoryLen--;
            alpha = std::max(alpha, score);
        }
        undoMove(board, states, ply);
        if (stats.stopped) return alpha;
        
        if (alpha >= beta) { // prune
            if (states[ply].capturedPiece == NO_PIECE) { // quiet move
                killers[1][ply] = killers[0][ply];
                killers[0][ply] = move;
            }
            break;
        }
    }

    if (legalmoves == 0) { // checkmate or stalemate
        bestScore = board.isInCheck(board.side) ? -(MATE_SCORE - ply) : 0;
        storeTT(board.key, depth, bestScore, TT_EXACT, 0);
        return bestScore;
    }

    TTFlag flag = TT_EXACT;
    if (bestScore <= alphaOrig) {
        flag = TT_ALPHA;
    }
    else if (bestScore >= beta) {
        flag = TT_BETA;
    }
    storeTT(board.key, depth, bestScore, flag, bestMove);

    return bestScore;
}

int quiescenceSearch(
    Board& board, 
    StateInfo* states, 
    int alpha, 
    int beta, 
    int ply,
    TimeManager& timeManager, 
    SearchStats& stats) 
{
    
    if (stats.stopped) return alpha;
    
    stats.qnodes++;
    if ((stats.qnodes & 4095) == 0 && timeManager.isExpired()) {
        stats.stopped = true;
        return alpha;
    }

    stats.seldepth = std::max(stats.seldepth, ply);
        
    int alphaOrig = alpha;

    // for qsearch we use a depth of QS_DEPTH to distinguish from the main search
    int ttScore;
    if (probeTT(board.key, QS_DEPTH, ply, alpha, beta, ttScore)) {
        stats.ttHits++;
        return ttScore;
    }
    
    bool inCheck = board.isInCheck(board.side);

    int bestScore = -INF;

    int standPat = -INF;

    if (!inCheck) {
        standPat = evaluate(board);
        bestScore = standPat;
        if (bestScore >= beta) return bestScore;
        alpha = std::max(alpha, bestScore);
    }
    
    Move bestMove = 0;

    // here we generate all the moves if we are in check
    // that is used to assert if we are in a stalemate or in a checkmate
    MoveList moves = inCheck ? generateMoves(board) : generateTacticalMoves(board);
    Move ttMove = getTTMove(board.key);
    scoreMoves(board, moves, ttMove, ply, killers);

    int legalmoves = 0;

    for (int i = 0; i < moves.count; i++) {
        pickBest(moves, i);
        Move move = moves.moves[i];

        // delta pruning: skip captures that can't raise alpha even in best case
        if (!inCheck && moveType(move) != PROMOTION) {
            int victimType = board.squares[moveTo(move)] & 7;
            if (moveType(move) == EN_PASSANT) victimType = PAWN;
            int captureGain = PIECE_VALUES_MG[victimType];
            if (standPat + captureGain + DELTA_MARGIN < alpha) continue;
        }

        doMove(board, move, states, ply);

        if (!board.isInCheck(board.side ^ 1)) {
            legalmoves++;
            int score = -quiescenceSearch(board, states, -beta, -alpha, ply + 1, timeManager, stats);
            if (score > bestScore) {
                bestScore = score;
                bestMove = move;
            }
            alpha = std::max(alpha, score);
        }
        undoMove(board, states, ply);
        if (stats.stopped) return alpha;
        
        if (alpha >= beta) {
            break;
        }
    }

    if (inCheck && legalmoves == 0) {
        bestScore = -(MATE_SCORE - ply); // checkmate
        storeTT(board.key, QS_DEPTH, bestScore, TT_EXACT, 0);
        return bestScore;
    }

    TTFlag flag = TT_EXACT;
    if (bestScore <= alphaOrig) {
        flag = TT_ALPHA;
    }
    else if (bestScore >= beta) {
        flag = TT_BETA;
    }
    storeTT(board.key, QS_DEPTH, bestScore, flag, bestMove);

    return bestScore;
}
