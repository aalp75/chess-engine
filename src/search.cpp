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
    TODO: too much duplicate code in findBestMove and negamax
    TODO: add a timeManager for the depth depeening

*/

constexpr int INF = 100'000;
constexpr int MAX_QDEPTH = 50;

int killers[2][256];

Move findBestMove(Board& board, int maxDepth, TimeManager& timeManager, SearchStats& stats) {

    StateInfo states[256];
    Move bestMove = 0;
    int bestScore = -INF;

    for (int depth = 1; depth <= maxDepth; depth++) {

        memset(killers, 0, sizeof(killers));

        MoveList moves = generateMoves(board);

        Key key = board.key;
        Move ttMove = 0;
        TTEntry& entry = tt[key & (TT_SIZE - 1)];
        if (entry.key == key) {
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
            if (!board.isInCheck(board.turn ^ 1)) {
                int score = -negamax(
                    board, 
                    states, 
                    depth - 1, 
                    -INF, 
                    -currentBestScore, 
                    ply + 1, 
                    timeManager,
                    stats
                );
                if (score > currentBestScore) {
                    currentBestScore = score;
                    currentBestMove = move;
                    if (currentBestScore >= INF - 100) break;
                }
            }
            undoMove(board, states, ply);
        }
        if (currentBestMove != 0 && !timeManager.isExpired()) {
            bestMove = currentBestMove;
            bestScore = currentBestScore;
            stats.depth = depth;
        }

        if (timeManager.isExpired()) break;
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
    stats.nodes++;

    if ((stats.nodes & 4095) == 0 && timeManager.isExpired()) return 0;

    if (depth == 0) {
        return quiescenceSearch(board, states, alpha, beta, ply + 1, 0, timeManager, stats);
    }

    Key key = board.key;
    int ttScore;
    if (probeTT(key, depth, alpha, beta, ttScore)) {
        stats.ttHits++;
        return ttScore;
    }

    if (depth >= 3 && !board.isInCheck(board.turn)) {

        doNullMove(board, states, ply);
        int nullScore = -negamax(
            board, 
            states, 
            depth - 3, 
            -beta, 
            -beta + 1, 
            ply + 1, 
            timeManager, stats
        );
        undoNullMove(board, states, ply);

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
        if (board.isInCheck(board.turn ^ 1)) {
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
        legal += 1;
        if (score >= beta) { // prune
            if (states[ply].capturedPiece == NO_PIECE) { // quiet move
                killers[1][ply] = killers[0][ply];
                killers[0][ply] = move;
            }
            storeTT(key, depth, beta, TT_BETA, move);
            return beta;
        }
        if (score > alpha) {
            bestMove = move;
            alpha = score;
        }
    }

    if (legal == 0) { // checkmate or stalemate
        int score = board.isInCheck(board.turn) ? -(INF - ply) : 0;
        storeTT(key, depth, score, TT_EXACT, 0);
        return score;
    }

    TTFlag flag = TT_EXACT;
    if (alpha <= originalAlpha) {
        flag = TT_ALPHA;
    }

    storeTT(key, depth, alpha, flag, bestMove);
    return alpha;
}

int quiescenceSearch(Board& board, StateInfo* states, int alpha, int beta, int ply, int qdepth, TimeManager& timeManager, SearchStats& stats) {

    if ((stats.qnodes & 4095) == 0 && timeManager.isExpired()) return 0;

    int eval = evaluate(board);
    if (eval >= beta) {
        return beta;
    }
    if (qdepth > MAX_QDEPTH) {
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
        if (!board.isInCheck(board.turn ^ 1)) {
            int score = -quiescenceSearch(board, states, -beta, -alpha, ply + 1, qdepth + 1, timeManager, stats);
            if (score >= beta) { 
                undoMove(board, states, ply); 
                return beta;
            }
            alpha = std::max(score, alpha);
        }
        undoMove(board, states, ply);
    }
    return alpha;

}
