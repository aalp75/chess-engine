#include <cstdint>

#include "evaluate.h"
#include "board.h"
#include "constants.h"
#include "pieceSquareTable.h"

int evaluate(const Board& board) {
    int score = 0;

    // split the evaluation for mid game and end game for king

    // end game is defined either:
    // 1 - both side have no queens
    // 2 - every side has a queen and no more than a minor piece

    bool endGame = false;

    int minorPieces[2] = {
        __builtin_popcountll(board.bitboards[BISHOP][WHITE]) + 
        __builtin_popcountll(board.bitboards[KNIGHT][WHITE]),

        __builtin_popcountll(board.bitboards[BISHOP][BLACK]) +
        __builtin_popcountll(board.bitboards[KNIGHT][BLACK])
    };
    int rooks[2] = {
        __builtin_popcountll(board.bitboards[ROOK][WHITE]),
        __builtin_popcountll(board.bitboards[ROOK][BLACK])
    };

    int queens[2] = {
        __builtin_popcountll(board.bitboards[QUEEN][WHITE]),
        __builtin_popcountll(board.bitboards[QUEEN][BLACK])
    };

    if (queens[WHITE] == 0 && queens[BLACK] == 0) {
        endGame = true;
    }

    if (rooks[WHITE] == 0 && rooks[BLACK] == 0 && 
        queens[WHITE] == 1 && queens[BLACK] == 1 && 
        minorPieces[WHITE] <= 1 && minorPieces[BLACK] <= 1) {
        endGame = true;
    }

    for (int piece = PAWN; piece <= KING; piece++) {
        const int* pst = PST[piece];

        if (piece == KING && endGame) {
            pst = PST[KING_END];
        }

        uint64_t positions = board.bitboards[piece][WHITE];

        while (positions) {
            int square = __builtin_ctzll(positions);
            int square_flip = (7 - square / 8) * 8 + (square % 8);
            score  += PIECE_VALUES[piece] + pst[square_flip];
            positions &= positions - 1;
        }

        positions = board.bitboards[piece][BLACK];
        while (positions) {
            int square = __builtin_ctzll(positions);
            score -= PIECE_VALUES[piece] + pst[square];
            positions &= positions - 1;
        }
    }
    return board.turn == WHITE ? score : -score;
}