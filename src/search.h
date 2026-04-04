#pragma once

#include<vector>

#include "board.h"
#include "moves.h"
#include "timeManager.h"

/**
 * TODO:
 * - clean the code
 * - rename the function with shorter name
 * - less arguments in the function
 * - add more comments to readibility
 * - better handling of this gameHistory
 */

extern Key gameHistory[1024];
extern int gameHistoryLen;

/**
 * nodes = positions evaluated in main search (depth > 0)
 * qnodes = positions evaluated in qsearch (depth <= 0)
 */

struct SearchStats {
    int       score    = 0;
    int       depth    = 0;
    int       seldepth = 0;
    long long nodes    = 0;
    long long qnodes   = 0;
    long long ttHits   = 0;
    bool      stopped  = false;
};

Move findBestMove(
    Board& board, 
    int depth, 
    TimeManager& timeManager, 
    SearchStats& stats, 
    bool useQSearch
);

int negamax(
    Board& board, 
    StateInfo* states, 
    int depth, 
    int alpha, 
    int beta, 
    int ply, 
    TimeManager& timeManager, 
    SearchStats& stats,
    bool useQSearch,
    bool nullMove = false
);

int quiescenceSearch(
    Board& board, 
    StateInfo* state, 
    int alpha, 
    int beta, 
    int ply, 
    TimeManager& timeManager, 
    SearchStats& stats
);
