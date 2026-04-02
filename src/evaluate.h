#pragma once

#include "board.h"

constexpr int PIECE_VALUES_MG[7] = {
    0,    // NO PIECE
    82,   // PAWN
    337,  // KNIGHT
    365,  // BISHOP
    477,  // ROOK
    1025, // QUEEN
    0     // KING
};

constexpr int PIECE_VALUES_EG[7] = {
    0,   // NO PIECE
    94,  // PAWN
    281, // KNIGHT
    297, // BISHOP
    512, // ROOK
    936, // QUEEN
    0    //KING
};

int evaluate(const Board& board);