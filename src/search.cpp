#include "search.h"

#include <algorithm>
#include <cmath>
#include <cstring>

#include "constants.h"
#include "evaluate.h"
#include "magic.h"
#include "moveList.h"
#include "moveOrdering.h"
#include "moves.h"
#include "transpositionTable.h"

// Search hyperparameters

constexpr int ASPIRATION_DELTA     = 25;
constexpr int RFP_MARGIN           = 75;
constexpr int RFP_IMPROVING_MARGIN = 55;
constexpr int FUTILITY_MARGIN[7]   = { 0, 100, 175, 275, 375, 475, 575 }; // margin per depth
constexpr int LMP_COUNT[2][6]      = {
    { 0, 3,  6, 10, 15, 21 },
    { 0, 5, 10, 16, 22, 28 },
};
constexpr int SEE_QUIET_MARGIN     = -60;   // SEE threshold for quiet move pruning
constexpr int SEE_CAPTURE_MARGIN   = -100;  // SEE threshold for capture pruning
constexpr int HISTORY_PRUNE_MARGIN = -3000; // prune quiet moves with very bad history
constexpr int QSEARCH_CHECK_LIMIT  = 4;     // max plies of check extensions in qsearch

// History tables helpers

void Searcher::updateHistory(int side, int from, int to, int bonus) {
    history[side][from][to] += bonus - history[side][from][to] * std::abs(bonus) / 8192;
}

void Searcher::updateCaptureHistory(int attacker, int to, int victim, int bonus) {
    captureHistory[attacker][to][victim] +=
        bonus - captureHistory[attacker][to][victim] * std::abs(bonus) / 8192;
}

Move Searcher::findBestMove(int maxDepth) {

    board.ply = 0;

    int bestScore = -INF;
    Move bestMove  = 0;

    for (int depth = 1; depth <= maxDepth; depth++) {

        Move currentBestMove = bestMove;
        int currentBestScore = -INF;

        std::fill(evalStack, evalStack + 256, -INF);

        // Aspiration window
        int delta = ASPIRATION_DELTA;
        int alpha = (depth >= 4) ? bestScore - delta : -INF;
        int beta  = (depth >= 4) ? bestScore + delta :  INF;

        while (true) {
            int localAlpha    = alpha;
            Move thisBestMove = 0;
            int thisBestScore = -INF;
            int legalCount    = 0;

            MoveList moves = generateMoves(board);
            Move ttMove = getTTMove(board.key);
            scoreMoves(board, moves, ttMove, 0, killers, history, captureHistory, countermove);

            for (int i = 0; i < moves.count; i++) {
                pickBest(moves, i);
                Move move = moves.moves[i];
                doMove(board, move);

                if (!board.isInCheck(board.side ^ 1)) {
                    gameHistory[gameHistoryLen++] = board.key;
                    int score;

                    // Full search on first legal move
                    if (legalCount == 0) {
                        score = -search(depth - 1, -beta, -localAlpha,
                                        true, false, 0, move);
                    }
                    else {
                        // null-window on other moves
                        score = -search(depth - 1, -localAlpha - 1, -localAlpha,
                                        false, false, 0, move);
                        // If the score beats expectation, re-search with full window
                        if (score > localAlpha && score < beta) {
                            score = -search(depth - 1, -beta, -localAlpha,
                                            true, false, 0, move);
                        }
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

            // Fail low: widen alpha downward and retry
            if (depth >= 4 && thisBestScore <= alpha) {
                beta  = (alpha + beta) / 2;
                alpha = std::max(thisBestScore - delta, -INF);
                delta = delta * 3 / 2 + 5;
                continue;
            }
            // Fail high: widen beta upward and retry
            if (depth >= 4 && thisBestScore >= beta) {
                beta  = std::min(thisBestScore + delta, INF);
                delta = delta * 3 / 2 + 5;
                continue;
            }

            // Search completed within the window
            currentBestMove  = thisBestMove;
            currentBestScore = thisBestScore;
            break;
        }

        // skip the result of a not fully depth search
        if (stats.stopped && bestMove != 0) break;

        bestMove  = currentBestMove;
        bestScore = currentBestScore;
        stats.depth = depth;
        stats.score = bestScore;

        // No need to search deeper once a forced mate is confirmed.
        if (bestScore > MATE_BOUND) break;
    }

    return bestMove;
}

void Searcher::iterativeDeepening(int maxDepth) {
    // TODO
}

int Searcher::search(int depth, int alpha, int beta,
                     bool isPV, bool nullMove, Move excludedMove, Move prevMove) {
    if (stats.stopped) return alpha;

    // Threefold repetition detection
    for (int i = gameHistoryLen - 3; i >= 0; i -= 2) {
        if (gameHistory[i] == board.key) return 0;
    }

    bool inCheck = board.isInCheck(board.side);

    if (depth == 0) {
        if (!useQSearch) return eval::evaluate(board);

        if (inCheck) depth = 1;
        else         return qSearch(alpha, beta, board.ply);
    }

    stats.nodes++;
    if ((stats.nodes & 4095) == 0 && tm.isExpired()) {
        stats.stopped = true;
        return alpha;
    }

    int ply = board.ply;

    if (ply >= MAX_PLY) return eval::evaluate(board);

    // Mate distance pruning
    alpha = std::max(alpha, -(MATE_SCORE - ply));
    beta  = std::min(beta,   (MATE_SCORE - ply + 1));
    if (alpha >= beta) return alpha;

    int alphaOrig = alpha;
    Move bestMove = 0;

    // Transposition table probe
    int ttScore = -INF;
    if (excludedMove == 0) {
        if (probeTT(board.key, depth, ply, alpha, beta, ttScore)) {
            stats.ttHits++;
            return ttScore;
        }
        ttScore = getTTScore(board.key);
    }
    Move ttMove = (excludedMove == 0) ? getTTMove(board.key) : 0;

    // Static evaluation used for forward pruning
    int eval = inCheck ? -INF : eval::evaluate(board);

    evalStack[ply] = eval;
    bool improving = !inCheck && ply >= 2 && evalStack[ply - 2] != -INF && eval > evalStack[ply - 2];

    // Reverse Futility Pruning
    bool hasBothSidesMaterial = (board.byColorBB[WHITE] & ~board.byTypeBB[KING])
                               && (board.byColorBB[BLACK] & ~board.byTypeBB[KING]);
    if (!isPV && !inCheck && depth <= 7 && excludedMove == 0 && hasBothSidesMaterial) {
        int rfpMargin = improving ? RFP_IMPROVING_MARGIN : RFP_MARGIN;
        if (eval - rfpMargin * depth >= beta) return eval;
    }

    // Null move pruning
    // move hasNonPawnMaterial to be a board method
    bool hasNonPawnMaterial = (board.byTypeBB[KNIGHT] | board.byTypeBB[BISHOP] |
                                board.byTypeBB[ROOK]   | board.byTypeBB[QUEEN])
                               & board.byColorBB[board.side];
    if (!isPV && !nullMove && !inCheck && depth >= 3 && hasNonPawnMaterial
        && excludedMove == 0 && eval >= beta) {
        int R = 3 + depth / 6 + std::min((eval - beta) / 200, 3) + improving;
        doNullMove(board);
        int nullScore = -search(depth - R - 1, -beta, -beta + 1, false, true, 0, 0);
        undoNullMove(board);
        if (stats.stopped) return alpha;
        if (nullScore >= beta) return beta;
    }

    // Singular Extension
    int singularExtension = 0;
    if (!inCheck && depth >= 6 && excludedMove == 0 && ttMove != 0
        && ttScore != -INF
        && getTTFlag(board.key) == TT_LOWER_BOUND
        && getTTDepth(board.key) >= depth - 3) {

        int singularBeta  = ttScore - 2 * depth;
        int singularDepth = (depth - 1) / 2;

        int sScore = search(singularDepth, singularBeta - 1, singularBeta,
                            false, false, ttMove, prevMove);

        if (sScore < singularBeta) {
            singularExtension = 1;
        }
        else if (singularBeta >= beta) {
            return singularBeta; // multi-cut
        }
    }

    MoveList moves = generateMoves(board);
    scoreMoves(board, moves, ttMove, ply, killers, history, captureHistory, countermove, prevMove);

    int  bestScore   = -INF;
    int  legalmoves  = 0;
    bool canFutility = !isPV && !inCheck && depth <= 6
                       && alpha > -MATE_BOUND && beta < MATE_BOUND;

    Move quietsTried[64];
    Move capturesTried[64];
    int quietsCount   = 0;
    int capturesCount = 0;

    for (int i = 0; i < moves.count; i++) {
        pickBest(moves, i);
        Move move = moves.moves[i];

        if (move == excludedMove) continue;

        int  type        = moveType(move);
        int  to          = moveTo(move);
        int  from        = moveFrom(move);
        int  victim      = (type == EN_PASSANT) ? PAWN
                         : (type == CASTLING)   ? NO_PIECE_TYPE
                         : (board.squares[to] & 7);
        bool isCapture   = (victim != NO_PIECE_TYPE);
        bool isPromotion = (type == PROMOTION);
        bool isQuiet     = !isCapture && !isPromotion;

        // Late move pruning
        if (!isPV && !inCheck && depth <= 5 && isQuiet
            && legalmoves >= LMP_COUNT[improving][depth]) continue;

        // Futility pruning
        if (canFutility && isQuiet && legalmoves > 0
            && eval + FUTILITY_MARGIN[depth] <= alpha) continue;

        // History pruning
        if (!isPV && !inCheck && depth <= 3 && isQuiet && legalmoves > 0
            && history[board.side][from][to] < HISTORY_PRUNE_MARGIN * depth) continue;

        // SEE pruning
        if (!isPV && !inCheck && depth <= 8 && move != ttMove) {
            int seeScore = board.see(move);
            if (isCapture && seeScore < SEE_CAPTURE_MARGIN * depth) continue;
            if (isQuiet   && seeScore < SEE_QUIET_MARGIN   * depth) continue;
        }

        doMove(board, move);

        if (!board.isInCheck(board.side ^ 1)) {
            legalmoves++;
            gameHistory[gameHistoryLen++] = board.key;

            if (isQuiet)   quietsTried[quietsCount++]     = move;
            if (isCapture) capturesTried[capturesCount++] = move;

            // Extensions
            int extension = (move == ttMove) ? singularExtension : 0;
            if (extension == 0 && board.isInCheck(board.side) && depth <= 10)
                extension = 1;

            int newDepth = depth - 1 + extension;
            int score;

            // LMR
            if (depth >= 3 && legalmoves >= 3 && isQuiet && !inCheck) {
                int reduction = LMR_TABLE[std::min(depth, 63)][std::min(legalmoves, 63)];
                if (!isPV)      reduction++;
                if (!improving) reduction++;
                reduction -= history[board.side][from][to] / 4096;
                reduction  = std::clamp(reduction, 0, newDepth - 1);

                score = -search(newDepth - reduction, -alpha - 1, -alpha,
                                false, false, 0, move);

                if (score > alpha && reduction > 0)
                    score = -search(newDepth, -alpha - 1, -alpha,
                                    false, false, 0, move);

                if (isPV && score > alpha)
                    score = -search(newDepth, -beta, -alpha,
                                    true, false, 0, move);
            }
            else if (isPV && legalmoves > 1) {
                score = -search(newDepth, -alpha - 1, -alpha,
                                false, false, 0, move);
                if (score > alpha && score < beta)
                    score = -search(newDepth, -beta, -alpha,
                                    true, false, 0, move);
            }
            else {
                score = -search(newDepth, -beta, -alpha,
                                isPV && legalmoves == 1, false, 0, move);
            }

            gameHistoryLen--;

            if (score > bestScore) { bestScore = score; bestMove = move; }
            alpha = std::max(alpha, score);
        }
        undoMove(board);
        if (stats.stopped) return alpha;

        if (alpha >= beta) {
            int bonus = std::min(depth * depth, 512);

            if (isQuiet) {
                updateHistory(board.side, from, to, bonus);
                for (int j = 0; j < quietsCount - 1; j++)
                    updateHistory(board.side,
                                  moveFrom(quietsTried[j]), moveTo(quietsTried[j]), -bonus);
                killers[1][ply] = killers[0][ply];
                killers[0][ply] = move;
                if (prevMove != 0)
                    countermove[board.side][moveFrom(prevMove)][moveTo(prevMove)] = move;
            }
            if (isCapture) {
                updateCaptureHistory(board.squares[from] & 7, to, victim, bonus);
                for (int j = 0; j < capturesCount - 1; j++) {
                    int pFrom = moveFrom(capturesTried[j]);
                    int pTo   = moveTo(capturesTried[j]);
                    updateCaptureHistory(board.squares[pFrom] & 7, pTo,
                                         board.squares[pTo] & 7, -bonus);
                }
            }
            break;
        }
    }

    // Checkmate or stalemate
    if (legalmoves == 0 && excludedMove == 0) {
        bestScore = inCheck ? -(MATE_SCORE - ply) : 0;
        storeTT(board.key, depth, ply, bestScore, TT_EXACT, 0);
        return bestScore;
    }

    // Store TT result
    if (excludedMove == 0) {
        TTFlag flag = TT_EXACT;
        if      (bestScore <= alphaOrig) flag = TT_UPPER_BOUND;
        else if (bestScore >= beta)      flag = TT_LOWER_BOUND;
        storeTT(board.key, depth, ply, bestScore, flag, bestMove);
    }

    return bestScore;
}

int Searcher::qSearch(int alpha, int beta, int ply, int qDepth) {
    if (stats.stopped) return alpha;

    stats.qnodes++;

    if ((stats.qnodes & 4095) == 0 && tm.isExpired()) {
        stats.stopped = true;
        return alpha;
    }

    stats.seldepth = std::max(stats.seldepth, ply);

    if (ply >= MAX_PLY) return eval::evaluate(board);

    bool inCheck = board.isInCheck(board.side);

    if (inCheck && qDepth >= QSEARCH_CHECK_LIMIT) {
        MoveList evasions = generateMoves(board);
        bool hasLegal = false;
        for (int i = 0; i < evasions.count; i++) {
            doMove(board, evasions.moves[i]);
            if (!board.isInCheck(board.side ^ 1)) hasLegal = true;
            undoMove(board);
            if (hasLegal) break;
        }
        if (!hasLegal) return -(MATE_SCORE - ply);
        return eval::evaluate(board);
    }

    int alphaOrig = alpha;

    int ttScore;
    if (probeTT(board.key, QS_DEPTH, ply, alpha, beta, ttScore)) {
        stats.ttHits++;
        return ttScore;
    }

    int bestScore = -INF;
    int standPat  = -INF;

    if (!inCheck) {
        standPat  = eval::evaluate(board);
        bestScore = standPat;
        if (bestScore >= beta) return bestScore;
        alpha = std::max(alpha, bestScore);
    }

    Move bestMove = 0;

    MoveList moves = inCheck ? generateMoves(board) : generateTacticalMoves(board);
    Move ttMove = getTTMove(board.key);
    scoreMoves(board, moves, ttMove, ply, killers, history, captureHistory, countermove);

    int legalmoves = 0;

    for (int i = 0; i < moves.count; i++) {
        pickBest(moves, i);
        Move move = moves.moves[i];
        int type  = moveType(move);
        int to    = moveTo(move);

        if (!inCheck) {
            if (type != PROMOTION && board.see(move) < 0) continue;

            if (type != PROMOTION) {
                int victim = (type == EN_PASSANT) ? PAWN : (board.squares[to] & 7);
                if (standPat + PIECE_VALUES_MG[victim] + DELTA_MARGIN < alpha) continue;
            }
        }

        doMove(board, move);

        if (!board.isInCheck(board.side ^ 1)) {
            legalmoves++;
            int nextQDepth = inCheck ? qDepth + 1 : qDepth;
            int score = -qSearch(-beta, -alpha, ply + 1, nextQDepth);
            if (score > bestScore) { bestScore = score; bestMove = move; }
            alpha = std::max(alpha, score);
        }
        undoMove(board);

        if (stats.stopped) return alpha;
        if (alpha >= beta) break;
    }

    if (legalmoves == 0) {
        if (inCheck) bestScore = -(MATE_SCORE - ply);
        TTFlag leafFlag = TT_EXACT;
        if      (bestScore <= alphaOrig) leafFlag = TT_UPPER_BOUND;
        else if (bestScore >= beta)      leafFlag = TT_LOWER_BOUND;
        storeTT(board.key, QS_DEPTH, ply, bestScore, leafFlag, 0);
        return bestScore;
    }

    TTFlag flag = TT_EXACT;
    if      (bestScore <= alphaOrig) flag = TT_UPPER_BOUND;
    else if (bestScore >= beta)      flag = TT_LOWER_BOUND;
    storeTT(board.key, QS_DEPTH, ply, bestScore, flag, bestMove);

    return bestScore;
}
