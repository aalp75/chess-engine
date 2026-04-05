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

int killers[2][256];

Key gameHistory[1024];
int gameHistoryLen = 0;

int simpleSee(const Board& board, Move move) {
    int from = moveFrom(move);
    int to = moveTo(move);

    int attackerType = board.squares[from] & 7;

    int victimType;
    if (moveType(move) == EN_PASSANT) victimType = PAWN;
    else {
        int capturedPiece = board.squares[to];
        if (capturedPiece == NO_PIECE) return 0;
        victimType = capturedPiece & 7;
    }

    return PIECE_VALUES_MG[victimType] - PIECE_VALUES_MG[attackerType];
}

inline Move getTTMove(Key key) {
    TTEntry& entry = tt[key & (TT_SIZE - 1)];
    return (entry.key == key) ? entry.bestMove : 0;
}

// Iterative deepening: search from depth 1 to maxDepth, keeping the best
// result from the last completed depth.
Move findBestMove(
    Board& board,
    int maxDepth,
    TimeManager& timeManager,
    SearchStats& stats,
    bool useQSearch)
{

    std::memset(killers, 0, sizeof(killers));

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
                int score = -search(board, states, depth - 1, -beta, -alpha, ply + 1, timeManager, stats, useQSearch);

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

        // checkmate has been found, no need to search deeper
        if (bestScore > MATE_BOUND) break;

    }

    return bestMove;
}

/**
 * Search is based on alpha beta pruning using negamax algoritm
 * 
 */
int search(
    Board& board,
    StateInfo* states,
    int depth,
    int alpha,
    int beta,
    int ply,
    TimeManager& timeManager,
    SearchStats& stats,
    bool useQSearch,
    bool nullMove)
{

    if (stats.stopped) return alpha;

    // detect threefold repetition (only check same-side positions)
    for (int i = gameHistoryLen - 3; i >= 0; i -= 2) {
        if (gameHistory[i] == board.key) return 0;
    }

    if (depth == 0) {
        if (useQSearch) {
            return qSearch(board, states, alpha, beta, ply, timeManager, stats);
        }
        else {
            return eval::evaluate(board);
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

    // null move pruning: skip our turn; if the result still beats beta, prune
    if (useQSearch && depth >= 3 && !nullMove && !board.isInCheck(board.side)) {
        doNullMove(board, states, ply);
        int nullScore = -search(board, states, depth - 3, -beta, -beta + 1, ply + 1, timeManager, stats, useQSearch, true);
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
            int score = -search(board, states, depth - 1, -beta, -alpha, ply + 1, timeManager, stats, useQSearch);
            if (score > bestScore) {
                bestScore = score;
                bestMove = move;
            }
            gameHistoryLen--;
            alpha = std::max(alpha, score);
        }
        undoMove(board, states, ply);
        if (stats.stopped) return alpha;

        if (alpha >= beta) { // beta cutoff
            if (states[ply].capturedPiece == NO_PIECE) { // quiet move: store killer
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
        flag = TT_UPPER_BOUND;
    }
    else if (bestScore >= beta) {
        flag = TT_LOWER_BOUND;
    }
    storeTT(board.key, depth, bestScore, flag, bestMove);

    return bestScore;
}

/**
 * Quiescence search: extend the initial search only on captures and checks 
 * It's to avoid the horizon effect
 * the function will be called once the depth of the search reaches 0
 */
int qSearch(
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

    int ttScore;
    if (probeTT(board.key, QS_DEPTH, ply, alpha, beta, ttScore)) {
        stats.ttHits++;
        return ttScore;
    }

    bool inCheck = board.isInCheck(board.side);

    int bestScore = -INF;
    int standPat = -INF;

    if (!inCheck) {
        standPat = eval::evaluate(board);
        bestScore = standPat;
        if (bestScore >= beta) return bestScore;
        alpha = std::max(alpha, bestScore);
    }

    Move bestMove = 0;

    // generate all moves when in check (to detect checkmate), captures only otherwise
    MoveList moves = inCheck ? generateMoves(board) : generateTacticalMoves(board);
    Move ttMove = getTTMove(board.key);
    scoreMoves(board, moves, ttMove, ply, killers);

    int legalmoves = 0;

    for (int i = 0; i < moves.count; i++) {
        pickBest(moves, i);
        Move move = moves.moves[i];

        if (!inCheck && moveType(move) != PROMOTION && simpleSee(board, move) < 0)
            continue;

        // delta pruning: skip captures that can't raise alpha even in best case
        if (!inCheck && moveType(move) != PROMOTION) {
            int victimType  = board.squares[moveTo(move)] & 7;
            if (moveType(move) == EN_PASSANT) victimType = PAWN;
            int captureGain = PIECE_VALUES_MG[victimType];

            // existing: skip if capture can't raise alpha
            if (standPat + captureGain + DELTA_MARGIN < alpha) continue;

            // new: skip losing captures (attacker worth more than victim)
            int attackerType = board.squares[moveFrom(move)] & 7;
            if (PIECE_VALUES_MG[attackerType] > PIECE_VALUES_MG[victimType] + DELTA_MARGIN) continue;
        }

        doMove(board, move, states, ply);

        if (!board.isInCheck(board.side ^ 1)) {
            legalmoves++;
            int score = -qSearch(board, states, -beta, -alpha, ply + 1, timeManager, stats);
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
        flag = TT_UPPER_BOUND;
    }
    else if (bestScore >= beta) {
        flag = TT_LOWER_BOUND;
    }
    storeTT(board.key, QS_DEPTH, bestScore, flag, bestMove);

    return bestScore;
}