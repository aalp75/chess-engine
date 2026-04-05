#include "evaluate.h"

#include <cstdint>

#include "board.h"
#include "constants.h"
#include "pieceSquareTable.h"
#include "utils.h"

// The evaluation function that is used to static evaluate the board.
//
// The logis is mostly based on 2 references:
// - https://www.chessprogramming.org/PeSTO%27s_Evaluation_Function
// - https://www.chessprogramming.org/CPW-Engine_eval

static Bitboard PASSED_MASK_WHITE[64];
static Bitboard PASSED_MASK_BLACK[64];
static Bitboard ADJACENT_FILES[8];

void eval::initialize() {
    // initialize the passed pawn mask
    // "A passed pawn has no opponent pawns in front on the same or adjacent files."
    // We precompute a mask to fast check if a pawn is passed
    // for example the passed pawn mask is something 
    // 00011100
    // 00011100
    // 0000p000
    // 00000000
    // ........

    for (int file = 0; file < FILE_NB; file++) {
        ADJACENT_FILES[file] = 0;

        if (file > 0) {
            ADJACENT_FILES[file] |= FILES[file - 1];
        }
        if (file < 7) {
            ADJACENT_FILES[file] |= FILES[file + 1];
        }
    }

    for (int square = 0; square < SQUARE_NB; square++) {
        int rank = square / FILE_NB;
        int file = square % FILE_NB;

        Bitboard fileMask = FILES[file] | ADJACENT_FILES[file];

        Bitboard above = 0;
        for (int r = rank + 1; r < RANK_NB; r++) {
            above |= RANKS[r];
        }
        PASSED_MASK_WHITE[square] = fileMask & above;

        Bitboard below = 0;
        for (int r = 0; r < rank; r++) {
            below |= RANKS[r];
        }
        PASSED_MASK_BLACK[square] = fileMask & below;
    }
}

inline int flipSquare(int square) {
    return square ^ 56;
}

void evalMaterialAndPst(const Board& board, int& mgScore, int& egScore) {
    Bitboard whiteBB = board.byColorBB[WHITE];
    Bitboard blackBB = board.byColorBB[BLACK];

    while (whiteBB) {
        int square = __builtin_ctzll(whiteBB);
        PieceType pieceType = static_cast<PieceType>(board.squares[square] & 7);

        int flippedSquare = flipSquare(square);

        mgScore += PIECE_VALUES_MG[pieceType] + PST_MG[pieceType][flippedSquare];
        egScore += PIECE_VALUES_EG[pieceType] + PST_EG[pieceType][flippedSquare];

        whiteBB &= whiteBB - 1;
    }

    while (blackBB) {
        int square = __builtin_ctzll(blackBB);
        PieceType pieceType = static_cast<PieceType>(board.squares[square] & 7);

        mgScore -= PIECE_VALUES_MG[pieceType] + PST_MG[pieceType][square];
        egScore -= PIECE_VALUES_EG[pieceType] + PST_EG[pieceType][square];

        blackBB &= blackBB - 1;
    }
}

void evalPawnStructure(const Board& board, int& mgScore, int& egScore) {
    Bitboard whitePawns = board.byTypeBB[PAWN] & board.byColorBB[WHITE];
    Bitboard blackPawns = board.byTypeBB[PAWN] & board.byColorBB[BLACK];

    // double pawns: penalization
    for (int file = 0; file < 8; file++) {
        int whites = popcount(whitePawns & FILES[file]);
        int blacks = popcount(blackPawns & FILES[file]);

        if (whites > 1) {
            mgScore -= (whites - 1) * DOUBLED_PENALTY_MG;
            egScore -= (whites - 1) * DOUBLED_PENALTY_EG;
        }
        if (blacks > 1) {
            mgScore += (blacks - 1) * DOUBLED_PENALTY_MG;
            egScore += (blacks - 1) * DOUBLED_PENALTY_EG;
        }
    }

    int wKingSq = lsb(board.byTypeBB[KING] & board.byColorBB[WHITE]);
    int bKingSq = lsb(board.byTypeBB[KING] & board.byColorBB[BLACK]);

    // white pawns: isolated & passed
    Bitboard bb = whitePawns;
    while (bb) {
        int square = lsb(bb);
        int rank   = square / FILE_NB;
        int file   = square % FILE_NB;

        // isolated
        if (!(whitePawns & ADJACENT_FILES[file])) {
            mgScore -= ISOLATED_PENALTY_MG;
            egScore -= ISOLATED_PENALTY_EG;
        }

        // passed
        if (!(blackPawns & PASSED_MASK_WHITE[square])) {
            mgScore += PASSED_BONUS_MG[rank];
            egScore += PASSED_BONUS_EG[rank];

            egScore += (7 - distance(wKingSq, square)) * 5; // bonus if the white king is close
            egScore -= (7 - distance(bKingSq, square)) * 3; // penalty if the black king is close
        }

        bb &= bb - 1;
    }

    bb = blackPawns;
    while (bb) {
        int square    = lsb(bb);
        int rankBlack = 7 - (square / 8);
        int file      = square % 8;

        // isolated
        if (!(blackPawns & ADJACENT_FILES[file])) {
            mgScore += ISOLATED_PENALTY_MG;
            egScore += ISOLATED_PENALTY_EG;
        }

        // passed
        if (!(whitePawns & PASSED_MASK_BLACK[square])) {
            mgScore -= PASSED_BONUS_MG[rankBlack];
            egScore -= PASSED_BONUS_EG[rankBlack];

            egScore += (7 - distance(wKingSq, square)) * 3; // white king close to black pawn = good for white
            egScore -= (7 - distance(bKingSq, square)) * 5; // black king escorts its pawn = bad for white
        }

        bb &= bb - 1;
    }
}

void evalPieceActivity(const Board& board, int& mgScore, int& egScore) {
    Bitboard whiteBishops = board.byTypeBB[BISHOP] & board.byColorBB[WHITE];
    Bitboard blackBishops = board.byTypeBB[BISHOP] & board.byColorBB[BLACK];

    // bonus for the bishop pair
    if (__builtin_popcountll(whiteBishops) >= 2) {
        mgScore += 30; // move to const value defined in constants.h
        egScore += 50;
    }

    if (__builtin_popcountll(blackBishops) >= 2) {
        mgScore -= 30;
        egScore -= 50;
    }
}

void evalKingSafety(const Board& board, int& mgScore, int& egScore) {
    
}

int computeStage(const Board& board, int mgScore, int egScore) {

    int gamePhase = __builtin_popcountll(board.byTypeBB[KNIGHT])
                  + __builtin_popcountll(board.byTypeBB[BISHOP])
                  + 2 * __builtin_popcountll(board.byTypeBB[ROOK])
                  + 4 * __builtin_popcountll(board.byTypeBB[QUEEN]);

    if (gamePhase > 24) gamePhase = 24;

    int mgWeight = gamePhase;
    int egWeight = 24 - mgWeight;

    return ((mgScore * mgWeight) + (egScore * egWeight)) / 24;
}

int eval::evaluate(const Board& board) {
    int mgScore = 0;
    int egScore = 0;

    evalMaterialAndPst(board, mgScore, egScore);
    evalPawnStructure(board, mgScore, egScore);
    evalPieceActivity(board, mgScore, egScore);
    evalKingSafety(board, mgScore, egScore);

    int score = computeStage(board, mgScore, egScore);
    return board.side == WHITE ? score : -score;
}