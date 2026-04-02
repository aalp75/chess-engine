#pragma once

#include <cstdint>
#include <string>

#include "constants.h"

namespace zobrist {
    void updateKeyPiece(Key& key, int piece, int square);
    void updateKeySide(Key& key);
    void updateKeyCastle(Key& key, int castle);
    void updateKeyEp(Key& key, int square);
};

struct Board {

    // data members

    Piece squares[SQUARE_NB] = { NO_PIECE };

    Bitboard byTypeBB[PIECE_TYPE_NB] = {}; // squares occupied by piece type
    Bitboard byColorBB[COLOR_NB] = {}; // squares occupied by color

    // for example if you want the SQUARE_NB occupied by the white knights you 
    // can do byTypeBB[KNIGHT] & byColorBB[WHITE]

    int  turn = WHITE;
    bool kingCastle[2];
    bool queenCastle[2];
    int  epSquare; // -1 if en passant is none
    Key  key;

    // method

    Board();
    Board(const std::string& fen);

    void clear();
    void loadFen(const std::string& fen);

    void display() const;

    void switchTurn();

    bool isSquareAttacked(int square, int color) const;
    bool isInCheck(int color) const;

    Key hash() const; // Zobrist hash
};