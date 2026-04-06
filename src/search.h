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

struct Searcher {

    // Data members
    Board board;
    TimeManager tm;
    SearchStats stats;

    bool useQSearch = true;

    // Game history for repetition detection
    Key gameHistory[1024] = {};
    int gameHistoryLen    = 0;

    // Searcher tables
    int killers[2][256];
    

    // method

    Searcher(Board board) : board(board) {}

    Move findBestMove(int maxDepth);
    void iterativeDeepening(int maxDepth);

    int search(int depth, int alpha, int beta, bool nullMove = false);
    int qSearch(int alpha, int beta, int ply);
};
