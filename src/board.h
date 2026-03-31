#pragma once

#include <cstdint>
#include <string>

#include "constants.h"

namespace zobrist {
    void updateKeyPiece(Key& key, int piece, int color, int square);
    void updateKeyside(Key& key);
    void updateKeyCastle(Key& key, int castle);
    void updateKeyEnPassant(Key& key, int square);
};

struct Board {

    // attributes

    Bitboard bitboards[7][2] = {};

    Bitboard wbPieces[2] = {0, 0};

    int pieceBoard[64];

    Bitboard occupied;

    int turn;

    bool kingCastle[2];
    bool queenCastle[2];

    bool enPassant;
    int enPassantSquare;

    Key key;

    // method

    Board();
    Board(std::string fen);

    void clear();
    void loadFen(std::string fen);

    void display() const;

    void switchTurn();

    bool isSquareAttacked(int square, int color) const;
    bool isInCheck(int color) const;

    Key hash() const; // Zobrist hash
};