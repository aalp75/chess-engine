#pragma once

#include "board.h"

constexpr int PIECE_VALUES[] = {
    0,
    100,
    300,
    330,
    500,
    900,
    0
};

int evaluate(const Board& board);