#pragma once

#include <cstdint>
#include <string>

#include "constants.h"

namespace zobrist { // move zobrist to a particular file
    void updateKeyPiece(Key& key, int piece, int square);
    void updateKeySide(Key& key);
    void updateKeyCastle(Key& key, int castle);
    void updateKeyEp(Key& key, int square);
};

struct StateInfo {
    int fromSquare;
    int toSquare;

    int capturedPiece;
    int movedPiece;

    int promoPiece;

    uint8_t castlingRights;

    int epSquare;
    int type;
    int capturedSquare;

    Key key;
};

struct Board {

    // data members

    Piece squares[SQUARE_NB] = { NO_PIECE };

    Bitboard byTypeBB[PIECE_TYPE_NB] = {}; // squares occupied by piece type
    Bitboard byColorBB[COLOR_NB] = {}; // squares occupied by color

    // for example if you want the SQUARE_NB occupied by the white knights you 
    // can do byTypeBB[KNIGHT] & byColorBB[WHITE]

    StateInfo states[256];
    int ply = 0;

    Color   side           = WHITE;
    uint8_t castlingRights = WHITE_OO | WHITE_OOO | BLACK_OO | BLACK_OOO;
    int     epSquare       = NO_SQUARE;
    Key     key            = 0;

    // method

    Board();
    Board(const std::string& fen);

    void clear();
    void loadFen(const std::string& fen);

    void display() const;

    void switchSide();

    bool isSquareAttacked(int square, int color) const;
    bool isInCheck(int color) const;

    PieceType lvaAttacker(int square, int side, Bitboard& occ) const;
    int see(Move move) const; // static exchange evaluation

    Key hash() const; // Zobrist hash
    
    inline Color getSide() const {
        return side;
    }
};