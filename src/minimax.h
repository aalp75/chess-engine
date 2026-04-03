/*
This code is used only for debugging and testing purposes

It allows us to compare the results of alpha-beta pruning
with those of an exhaustive minimax search
*/

#pragma once

#include "board.h"
#include "moves.h"
#include "constants.h"
#include "search.h"

Move findBestMoveMinimax(Board& board, int depth, SearchStats& stats);
int minimax(Board& board, StateInfo* states, int depth, int ply, int player);
//int quiescenceSearchMinimax(Board& board, StateInfo* states, int ply, int player);
