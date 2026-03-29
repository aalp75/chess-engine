#include<vector>
#include<algorithm>

#include "search.h"
#include "constants.h"
#include "moves.h"
#include "board.h"
#include "evaluate.h"
#include "moveOrdering.h"
#include "moveList.h"
#include "transpositionTable.h"

/*
    TODO: too much duplicate code in findBestMove and negamax
    TODO: add a timeManager for the depth depeening

*/

constexpr int INF = 100'000;
constexpr int MAX_QDEPTH = 10;

Move findBestMove(Board& board, int maxDepth, int& score, long long& nodes) {

    StateInfo states[256];
    Move bestMove = 0;
    int bestScore = -INF;

    for (int depth = 1; depth <= maxDepth; depth++) {

        MoveList moves = generateMoves(board);

        Key key = board.key;
        Move ttMove = 0;
        TTEntry& entry = tt[key & (TT_SIZE - 1)];
        if (entry.key == key) {
            ttMove = entry.bestMove;
        }
    
        std::sort(moves.moves, moves.moves + moves.count,
            [&](const Move m1, const Move m2) {
                if (m1 == ttMove) return true;
                if (m2 == ttMove) return false;
                return moveScore(board, m1) > moveScore(board, m2);
            }
        );

        int currentBestScore = -INF;
        Move currentBestMove = 0;
        int ply = 0;

        for (int i = 0; i < moves.count; i++) {
            Move move = moves.moves[i];
            doMove(board, move, states, ply);
            if (!board.isInCheck(board.turn ^ 1)) {
                int score = -negamax(board, states, depth - 1, -INF, -currentBestScore, ply + 1, nodes);
                if (score > currentBestScore) {
                    currentBestScore = score;
                    currentBestMove = move;
                }
            }
            undoMove(board, states, ply);
        }
        if (currentBestMove != 0) {
            bestMove = currentBestMove;
            bestScore = currentBestScore;
        }
    }

    score = bestScore;
    return bestMove;
}

int negamax(Board& board, StateInfo* states, int depth, int alpha, int beta, int ply, long long& nodes) {
    nodes++;

    if (depth == 0) {
        return quiescenceSearch(board, states, alpha, beta, ply + 1, 0, nodes);
    }


    Key key = board.key;
    int ttScore;
    if (probeTT(key, depth, alpha, beta, ttScore)) {
        return ttScore;
    }

    int originalAlpha = alpha;
    Move bestMove = 0;

    MoveList moves = generateMoves(board);
    Move ttMove = 0;
    TTEntry& entry = tt[key & (TT_SIZE - 1)];
    if (entry.key == key) {
        ttMove = entry.bestMove;
    }
    std::sort(moves.moves, moves.moves + moves.count,
        [&](const Move m1, const Move m2) {
            if (m1 == ttMove) return true;
            if (m2 == ttMove) return false;
            return moveScore(board, m1) > moveScore(board, m2);
        }
    );

    int legal = 0;

    for (int i = 0; i < moves.count; i++) {
        Move move = moves.moves[i];
        doMove(board, move, states, ply);
        if (board.isInCheck(board.turn ^ 1)) {
            undoMove(board, states, ply);   
            continue;
        }
        int score = -negamax(board, states, depth - 1, -beta, -alpha, ply + 1, nodes);
        undoMove(board, states, ply);   
        legal += 1;
        if (score >= beta) { // prune
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

int quiescenceSearch(Board& board, StateInfo* states, int alpha, int beta, int ply, int qdepth, long long& nodes) {
    nodes++;
    int eval = evaluate(board);
    if (eval >= beta) {
        return beta;
    }
    if (qdepth > MAX_QDEPTH) {
        return eval;
    }

    alpha = std::max(alpha, eval);

    MoveList moves = generateMoves(board);
    std::sort(moves.moves, moves.moves + moves.count,
        [&](const Move m1, const Move m2) {
            return moveScore(board, m1) > moveScore(board, m2);
        }
    );

    for (int i = 0; i < moves.count; i++) {
        Move move = moves.moves[i];
        int to = moveTo(move);
        int type = moveType(move);
        
        bool isCapture = (board.pieceBoard[to] != NO_PIECE) || type == EN_PASSANT;
        bool isPromotion = type == PROMOTION;

        if (!isCapture && !isPromotion) { // no capture & no promotions
            continue;
        }
        doMove(board, move, states, ply);
        if (!board.isInCheck(board.turn ^ 1)) {
            int score = -quiescenceSearch(board, states, -beta, -alpha, ply + 1, qdepth + 1, nodes);
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