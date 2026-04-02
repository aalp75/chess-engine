#include "zobrist.h"
#include "utils.h"

namespace zobrist {

    Key pieceKeys[PIECE_NB][SQUARE_NB];
    Key sideKey;
    Key castleKeys[16];
    Key epKeys[SQUARE_NB];

    bool initialized = false;

    void initialize() {
        if (initialized) return;

        for (int piece = 0; piece < PIECE_NB; piece++) {
            for (int square = 0; square < SQUARE_NB; square++) {
                pieceKeys[piece][square] = random64();
            }   
        }

        sideKey = random64();

        for (int d = 0; d < 16; d++) {
            castleKeys[d] = random64();
        }

        for (int square = 0; square < SQUARE_NB; square++) {
            epKeys[square] = random64();
        }

        initialized = true;
    }

    void updateKeyPiece(Key& key, int piece, int square) {
        key ^= pieceKeys[piece][square];
    }

    void updateKeySide(Key& key) {
        key ^= sideKey;
    }

    void updateKeyCastle(Key& key, int castlingRights) {
        key ^= castleKeys[castlingRights];
    }

    void updateKeyEp(Key& key, int square) {
        key ^= epKeys[square];
    }
}