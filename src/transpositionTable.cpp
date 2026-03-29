#include "transpositionTable.h"
#include "constants.h"

TTEntry tt[TT_SIZE];

// check if the key is in the table and what is its score
bool probeTT(Key key, int depth, int alpha, int beta, int& score) {
    TTEntry& entry = tt[key & (TT_SIZE - 1)];

    if (entry.key != key) {
        return false;
    }

    if (entry.depth < depth) {
        return false;
    }

    if (entry.flag == TT_EXACT) {
        score = entry.score;
        return true;
    }

    if (entry.flag == TT_ALPHA && entry.score <= alpha) {
        score = entry.score;
        return true;
    }

    if (entry.flag == TT_BETA && entry.score >= beta) {
        score = entry.score;
        return true;
    }

    return false;
}

void storeTT(Key key, int depth, int score, TTFlag flag, Move bestMove) {
    TTEntry& entry = tt[key & (TT_SIZE -1)];
    
    if (entry.key != key || depth >= entry.depth) {
        entry.key = key;
        entry.depth = depth;
        entry.score = score;
        entry.flag = flag;
        entry.bestMove = bestMove;
    }
}