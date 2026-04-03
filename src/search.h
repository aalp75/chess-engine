#pragma once

#include<vector>

#include "board.h"
#include "moves.h"
#include "timeManager.h"

struct SearchStats {
    long long score    = 0;
    long long depth    = 0;
    long long seldepth = 0;
    long long nodes    = 0;
    long long qnodes   = 0;
    long long ttHits   = 0;
    bool      stopped  = false;
};

Move findBestMove(Board& board, int depth, TimeManager& timeManager, SearchStats& stats);

int negamax(
    Board& board, 
    StateInfo* states, 
    int depth, 
    int alpha, 
    int beta, 
    int ply, 
    TimeManager& timeManager, 
    SearchStats& stats
);

int quiescenceSearch(
    Board& board, 
    StateInfo* state, 
    int alpha, 
    int beta, 
    int ply, 
    int qdepth, 
    TimeManager& timeManager, 
    SearchStats& stats
);
