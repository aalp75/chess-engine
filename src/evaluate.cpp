#include <cstdint>

#include "evaluate.h"
#include "board.h"
#include "constants.h"
#include "pieceSquareTable.h"

int evaluate(const Board& board) {
    int score = 0;

    // split the evaluation for mid game and end game for king

    // end game is defined when the total material on the board is low (below a treshold)

    int material[2] = {
        PIECE_VALUES[KNIGHT] * __builtin_popcountll(board.bitboards[KNIGHT][WHITE]) +
        PIECE_VALUES[BISHOP] * __builtin_popcountll(board.bitboards[BISHOP][WHITE]) +
        PIECE_VALUES[ROOK]   * __builtin_popcountll(board.bitboards[ROOK][WHITE])   +
        PIECE_VALUES[QUEEN]  * __builtin_popcountll(board.bitboards[QUEEN][WHITE]),

        PIECE_VALUES[KNIGHT] * __builtin_popcountll(board.bitboards[KNIGHT][BLACK]) +
        PIECE_VALUES[BISHOP] * __builtin_popcountll(board.bitboards[BISHOP][BLACK]) +
        PIECE_VALUES[ROOK]   * __builtin_popcountll(board.bitboards[ROOK][BLACK])   +
        PIECE_VALUES[QUEEN]  * __builtin_popcountll(board.bitboards[QUEEN][BLACK])
    };

    bool endGame = (material[WHITE] + material[BLACK]) < 1600;

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