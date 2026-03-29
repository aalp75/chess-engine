#pragma once

#include<vector>

#include "board.h"
#include "constants.h"
#include "moveList.h"

struct StateInfo {
    int fromSquare;
    int toSquare;

    int capturedPiece;
    int movedPiece;

    int promoPiece;

    bool kingCastle[2];
    bool queenCastle[2];

    bool enPassant;
    int enPassantSquare;
    int type;
    int capturedSquare;

    Key key;
};

void doMove(Board& board, Move move, StateInfo* states, int ply);
void undoMove(Board& board, StateInfo* states, int ply);

MoveList generateMoves(const Board& board);

void generatePawnMoves(const Board& board, MoveList& moves);
void generateKnightMoves(const Board& board, MoveList& moves);
void generateBishopMoves(const Board& board, MoveList& moves);
void generateRookMoves(const Board& board, MoveList& moves);
void generateQueenMoves(const Board& board, MoveList& moves);
void generateKingMoves(const Board& board, MoveList& moves);