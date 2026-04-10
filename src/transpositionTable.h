#pragma once

#include <cstdint>

#include "constants.h"
#include "moveList.h"

enum TTFlag {
    TT_EXACT,   
    TT_UPPER_BOUND,   // real score <= stored score (alpha was not raised)
    TT_LOWER_BOUND    // real score >= stored score (caused a beta cutoff)
};

struct TTEntry {
    Key key = 0;
    Move bestMove = 0;
    int depth = -1;
    int score = 0;
    TTFlag flag = TT_EXACT;
};

constexpr int TT_SIZE = 1 << 24;
extern TTEntry tt[TT_SIZE];

void clearTT();
bool probeTT(Key key, int depth, int ply, int alpha, int beta, int& score);
void storeTT(Key key, int depth, int ply, int score, TTFlag flag, Move bestMove);

inline Move getTTMove(Key key) {
    TTEntry& e = tt[key & (TT_SIZE - 1)];
    return (e.key == key) ? e.bestMove : 0;
}
inline int getTTScore(Key key) {
    TTEntry& e = tt[key & (TT_SIZE - 1)];
    return (e.key == key) ? e.score : -INF;
}
inline TTFlag getTTFlag(Key key) {
    TTEntry& e = tt[key & (TT_SIZE - 1)];
    return (e.key == key) ? e.flag : TT_UPPER_BOUND;
}
inline int getTTDepth(Key key) {
    TTEntry& e = tt[key & (TT_SIZE - 1)];
    return (e.key == key) ? e.depth : -1;
}