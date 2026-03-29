#pragma once

#include<vector>

#include "board.h"
#include "moves.h"

// TO DO: add move ordering

Move findBestMove(Board& board, int depth, int& score, long long& nodes);
int negamax(Board& board, StateInfo* states, int depth, int alpha, int beta, int ply, long long& nodes);
int quiescenceSearch(Board& board, StateInfo* state, int alpha, int beta, int ply, int qdepth, long long& nodes);
