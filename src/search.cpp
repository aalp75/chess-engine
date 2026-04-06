#include "search.h"

#include <algorithm>
#include <cstring>

#include "constants.h"
#include "evaluate.h"
#include "moveList.h"
#include "moveOrdering.h"
#include "moves.h"
#include "transpositionTable.h"

inline Move getTTMove(Key key) {
    TTEntry& entry = tt[key & (TT_SIZE - 1)];
    return (entry.key == key) ? entry.bestMove : 0;
}

static constexpr int ASPIRATION_DELTA = 25;

Move Searcher::findBestMove(int maxDepth) {
    std::memset(killers, 0, sizeof(killers));
    stats = SearchStats{};

    int bestScore = -INF;
    Move bestMove = 0;

    for (int depth = 1; depth <= maxDepth; depth++) {
        Move currentBestMove  = 0;
        int  currentBestScore = -INF;

        int delta = ASPIRATION_DELTA;
        int alpha = (depth > 4) ? bestScore - delta : -INF;
        int beta  = (depth > 4) ? bestScore + delta :  INF;

        while (true) {
            int  localAlpha   = alpha;
            Move thisBestMove = 0;
            int  thisBestScore = -INF;
            int  legalCount   = 0;

            MoveList moves  = generateMoves(board);
            Move     ttMove = getTTMove(board.key);
            scoreMoves(board, moves, ttMove, 0, killers);

            for (int i = 0; i < moves.count; i++) {
                pickBest(moves, i);
                Move move = moves.moves[i];
                doMove(board, move);

                if (!board.isInCheck(board.side ^ 1)) {
                    gameHistory[gameHistoryLen++] = board.key;
                    int score;

                    if (legalCount == 0) {
                        score = -search(depth - 1, -beta, -localAlpha);
                    } else {
                        score = -search(depth - 1, -localAlpha - 1, -localAlpha);
                        if (score > localAlpha && score < beta)
                            score = -search(depth - 1, -beta, -localAlpha);
                    }

                    gameHistoryLen--;
                    legalCount++;

                    if (score > thisBestScore) {
                        thisBestScore = score;
                        thisBestMove  = move;
                    }
                    if (score > localAlpha) localAlpha = score;
                }
                undoMove(board);
                if (stats.stopped) break;
            }

            if (stats.stopped) break;

            if (depth > 4 && thisBestScore <= alpha) {
                beta  = (alpha + beta) / 2;
                alpha = std::max(thisBestScore - delta, -INF);
                delta = delta * 3 / 2 + 5;
                continue;
            }
            if (depth > 4 && thisBestScore >= beta) {
                beta  = std::min(thisBestScore + delta, INF);
                delta = delta * 3 / 2 + 5;
                continue;
            }

            currentBestMove  = thisBestMove;
            currentBestScore = thisBestScore;
            break;
        }

        if (stats.stopped && bestMove != 0) break;

        bestMove  = currentBestMove;
        bestScore = currentBestScore;
        stats.depth = depth;
        stats.score = bestScore;

        if (bestScore > MATE_BOUND) break;
    }

    return bestMove;
}

void Searcher::iterativeDeepening(int maxDepth) {
    //findBestMove(maxDepth);
}

int Searcher::search(int depth, int alpha, int beta, bool nullMove) {
    if (stats.stopped) return alpha;

    // Threefold repetition detection
    for (int i = gameHistoryLen - 3; i >= 0; i -= 2)
        if (gameHistory[i] == board.key) return 0;

    int ply = board.ply;

    if (depth == 0) {
        if (useQSearch) return qSearch(alpha, beta, ply);
        else            return eval::evaluate(board);
    }

    stats.nodes++;
    if ((stats.nodes & 4095) == 0 && tm.isExpired()) {
        stats.stopped = true;
        return alpha;
    }

    if (ply >= 127) return eval::evaluate(board);

    int  alphaOrig = alpha;
    Move bestMove  = 0;

    int ttScore;
    if (probeTT(board.key, depth, ply, alpha, beta, ttScore)) {
        stats.ttHits++;
        return ttScore;
    }

    // Null move pruning: skip our turn; if the result still beats beta, prune
    if (useQSearch && depth >= 3 && !nullMove && !board.isInCheck(board.side)) {
        doNullMove(board);
        int nullScore = -search(depth - 3, -beta, -beta + 1, true);
        undoNullMove(board);
        if (stats.stopped) return alpha;
        if (nullScore >= beta) return beta;
    }

    MoveList moves  = generateMoves(board);
    Move     ttMove = getTTMove(board.key);
    scoreMoves(board, moves, ttMove, ply, killers);

    int bestScore  = -INF;
    int legalmoves = 0;

    for (int i = 0; i < moves.count; i++) {
        pickBest(moves, i);
        Move move = moves.moves[i];
        doMove(board, move);

        if (!board.isInCheck(board.side ^ 1)) {
            legalmoves++;
            gameHistory[gameHistoryLen++] = board.key;
            int score = -search(depth - 1, -beta, -alpha);
            gameHistoryLen--;
            if (score > bestScore) {
                bestScore = score;
                bestMove  = move;
            }
            alpha = std::max(alpha, score);
        }
        undoMove(board);

        if (stats.stopped) return alpha;

        if (alpha >= beta) {
            // killer: only update for quiet moves (not captures)
            if (board.states[board.ply].capturedPiece == NO_PIECE) {
                killers[1][ply] = killers[0][ply];
                killers[0][ply] = move;
            }
            break;
        }
    }

    if (legalmoves == 0) {
        bestScore = board.isInCheck(board.side) ? -(MATE_SCORE - ply) : 0;
        storeTT(board.key, depth, bestScore, TT_EXACT, 0);
        return bestScore;
    }

    TTFlag flag = TT_EXACT;
    if      (bestScore <= alphaOrig) flag = TT_UPPER_BOUND;
    else if (bestScore >= beta)      flag = TT_LOWER_BOUND;
    storeTT(board.key, depth, bestScore, flag, bestMove);

    return bestScore;
}

int Searcher::qSearch(int alpha, int beta, int ply) {
    if (stats.stopped) return alpha;

    stats.qnodes++;
    if ((stats.qnodes & 4095) == 0 && tm.isExpired()) {
        stats.stopped = true;
        return alpha;
    }

    stats.seldepth = std::max(stats.seldepth, ply);

    if (ply >= 127) return eval::evaluate(board);

    int alphaOrig = alpha;

    int ttScore;
    if (probeTT(board.key, QS_DEPTH, ply, alpha, beta, ttScore)) {
        stats.ttHits++;
        return ttScore;
    }

    bool inCheck  = board.isInCheck(board.side);
    int  bestScore = -INF;
    int  standPat  = -INF;

    if (!inCheck) {
        standPat  = eval::evaluate(board);
        bestScore = standPat;
        if (bestScore >= beta) return bestScore;
        alpha = std::max(alpha, bestScore);
    }

    Move     bestMove = 0;
    MoveList moves    = inCheck ? generateMoves(board) : generateTacticalMoves(board);
    Move     ttMove   = getTTMove(board.key);
    scoreMoves(board, moves, ttMove, ply, killers);

    int legalmoves = 0;

    for (int i = 0; i < moves.count; i++) {
        pickBest(moves, i);
        Move move = moves.moves[i];
        doMove(board, move);

        if (!board.isInCheck(board.side ^ 1)) {
            legalmoves++;
            int score = -qSearch(-beta, -alpha, ply + 1);
            if (score > bestScore) { bestScore = score; bestMove = move; }
            alpha = std::max(alpha, score);
        }
        undoMove(board);

        if (stats.stopped) return alpha;
        if (alpha >= beta) break;
    }

    if (inCheck && legalmoves == 0) {
        bestScore = -(MATE_SCORE - ply);
        storeTT(board.key, QS_DEPTH, bestScore, TT_EXACT, 0);
        return bestScore;
    }

    TTFlag flag = TT_EXACT;
    if      (bestScore <= alphaOrig) flag = TT_UPPER_BOUND;
    else if (bestScore >= beta)      flag = TT_LOWER_BOUND;
    storeTT(board.key, QS_DEPTH, bestScore, flag, bestMove);

    return bestScore;
}
