#pragma once

#include <cstdint>

#include "constants.h"
#include "moveList.h"

enum TTFlag {
    TT_EXACT,   
    TT_ALPHA,   // upper bound (real score <= stored score)
    TT_BETA     // lower bound (real score >= stored score)
};

struct TTEntry {
    Key key = 0;
    Move bestMove = 0;
    int depth = -1;
    int score = 0;
    TTFlag flag = TT_EXACT;
};

constexpr int TT_SIZE = 1 << 23;
extern TTEntry tt[TT_SIZE];

bool probeTT(Key key, int depth, int ply, int alpha, int beta, int& score);
void storeTT(Key key, int depth, int ply, int score, TTFlag flag, Move bestMove);