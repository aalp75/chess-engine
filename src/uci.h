#pragma once

#include <string>
#include <vector>

#include "board.h"
#include "search.h"
#include "constants.h"

struct SearchResult {
    int countMove     = 0;
    Move bestMove     = 0;
    int score         = 0;
    int depth         = 0;
    long long ms      = 0;
    SearchStats stats;
};

std::string sqToStr(int sq);
std::string moveToUci(const Move move);

void run(int depth, bool playSound);



