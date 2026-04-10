#include "transpositionTable.h"

#include <cstring>

#include "constants.h"

TTEntry tt[TT_SIZE];

void clearTT() {
    std::memset(tt, 0, sizeof(tt));
}

// check if the key is in the table and what is its score
bool probeTT(Key key, int depth, int ply, int alpha, int beta, int& score) {
    TTEntry& entry = tt[key & (TT_SIZE - 1)];

    // check collision and depth
    if (entry.key != key || entry.depth < depth) return false;

    score = entry.score;
    // convert from node-relative (stored) to root-relative (returned)
    if (score >  MATE_BOUND) score -= ply;
    if (score < -MATE_BOUND) score += ply;

    if (entry.flag == TT_EXACT) return true;
    if (entry.flag == TT_UPPER_BOUND && score <= alpha) return true;
    if (entry.flag == TT_LOWER_BOUND && score >= beta)  return true;

    return false;
}

void storeTT(Key key, int depth, int ply, int score, TTFlag flag, Move bestMove) {
    TTEntry& entry = tt[key & (TT_SIZE - 1)];

    if (entry.key != key || depth >= entry.depth) {
        // convert from root-relative to node-relative so mate distance is preserved
        if (score >  MATE_BOUND) score += ply;
        if (score < -MATE_BOUND) score -= ply;

        entry.key      = key;
        entry.depth    = depth;
        entry.score    = score;
        entry.flag     = flag;
        entry.bestMove = bestMove;
    }
}
