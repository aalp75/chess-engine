#pragma once

#include<vector>

#include "board.h"
#include "moves.h"

struct SearchStats {
    long long score  = 0;
    long long nodes  = 0;
    long long qnodes = 0;
    long long ttHits = 0;
};

Move findBestMove(Board& board, int depth, SearchStats& stats);
int negamax(Board& board, StateInfo* states, int depth, int alpha, int beta, int ply, SearchStats& stats);
int quiescenceSearch(Board& board, StateInfo* state, int alpha, int beta, int ply, int qdepth, SearchStats& stats);
