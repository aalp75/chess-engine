#include "transpositionTable.h"
#include "constants.h"

TTEntry tt[TT_SIZE];

// check if the key is in the table and what is its score
bool probeTT(Key key, int depth, int ply, int alpha, int beta, int& score) {
    TTEntry& entry = tt[key & (TT_SIZE - 1)];

    if (entry.key != key) return false;
    if (entry.depth < depth) return false;

    score = entry.score;
    if (score >  100'000 - 100) score -= ply;
    if (score < -100'000 + 100) score += ply;

    if (entry.flag == TT_EXACT) return true;
    if (entry.flag == TT_ALPHA && score <= alpha) return true;
    if (entry.flag == TT_BETA  && score >= beta)  return true;

    return false;
}

void storeTT(Key key, int depth, int ply, int score, TTFlag flag, Move bestMove) {
    TTEntry& entry = tt[key & (TT_SIZE - 1)];

    if (entry.key != key || depth >= entry.depth) {
        if (score >  100000 - 100) score += ply;
        if (score < -100000 + 100) score -= ply;
        entry.key = key;
        entry.depth = depth;
        entry.score = score;
        entry.flag = flag;
        entry.bestMove = bestMove;
    }
}
