#pragma once

#include <vector>
#include <cstring>
#include <cmath>
#include <algorithm>

#include "board.h"
#include "moves.h"
#include "timeManager.h"
#include "constants.h"

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
    Board       board;
    TimeManager tm;
    SearchStats stats;

    bool useQSearch = true;

    // Game history for repetition detection
    Key gameHistory[1024] = {};
    int gameHistoryLen    = 0;

    // search tables
    int killers[2][256]; 
    int history[2][64][64]; 
    int captureHistory[7][64][7];
    int countermove[2][64][64];
    int evalStack[256];
    int LMR_TABLE[64][64];

    Searcher(Board b) : board(b) {
        std::memset(killers,        0, sizeof(killers));
        std::memset(history,        0, sizeof(history));
        std::memset(captureHistory, 0, sizeof(captureHistory));
        std::memset(countermove,    0, sizeof(countermove));

        std::fill(evalStack, evalStack + 256, -INF);
        
        for (int d = 1; d < 64; d++)
            for (int m = 1; m < 64; m++)
                LMR_TABLE[d][m] = std::max(0, (int)(0.75 + std::log(d) * std::log(m) / 2.25));
    }

    Move findBestMove(int maxDepth);
    void iterativeDeepening(int maxDepth);

    int search(int depth, int alpha, int beta,
               bool isPV       = false,
               bool nullMove   = false,
               Move excludedMove = 0,
               Move prevMove   = 0);

    int qSearch(int alpha, int beta, int ply, int qDepth = 0);

    void updateHistory(int side, int from, int to, int bonus);
    void updateCaptureHistory(int attacker, int to, int victim, int bonus);
};
