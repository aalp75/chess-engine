#pragma once

#include<vector>

#include "board.h"
#include "constants.h"
#include "moveList.h"

void doMove(Board& board, Move move);
void undoMove(Board& board);

void doNullMove(Board& board);
void undoNullMove(Board& board);

MoveList generateMoves(const Board& board);
MoveList generateTacticalMoves(const Board& board);

void generatePawnMoves(const Board& board, MoveList& moves);
void generateKnightMoves(const Board& board, MoveList& moves);
void generateBishopMoves(const Board& board, MoveList& moves);
void generateRookMoves(const Board& board, MoveList& moves);
void generateQueenMoves(const Board& board, MoveList& moves);
void generateKingMoves(const Board& board, MoveList& moves);

void generateTacticalPawnMoves(const Board& board, MoveList& moves);
void generateTacticalKnightMoves(const Board& board, MoveList& moves);
void generateTacticalBishopMoves(const Board& board, MoveList& moves);
void generateTacticalRookMoves(const Board& board, MoveList& moves);
void generateTacticalQueenMoves(const Board& board, MoveList& moves);
void generateTacticalKingMoves(const Board& board, MoveList& moves);