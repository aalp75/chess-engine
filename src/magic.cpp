#include <cassert>
#include <cmath>
#include <iostream>
#include <random>
#include <vector>

#include "magic.h"
#include "constants.h"

Bitboard W_PAWN_ATTACKS[SQUARE_NB];
Bitboard B_PAWN_ATTACKS[SQUARE_NB];

Bitboard KNIGHT_ATTACKS[SQUARE_NB];

Bitboard BISHOP_MASK[SQUARE_NB];
Bitboard BISHOP_MAGIC[SQUARE_NB];
int      BISHOP_SHIFT[SQUARE_NB];
Bitboard BISHOP_ATTACKS[SQUARE_NB][512];

Bitboard ROOK_MASK[SQUARE_NB];
Bitboard ROOK_MAGIC[SQUARE_NB];
int      ROOK_SHIFT[SQUARE_NB];
Bitboard ROOK_ATTACKS[SQUARE_NB][4096];

Bitboard KING_ATTACKS[SQUARE_NB];

std::mt19937_64 rng(std::random_device{}());

// TO DO at some point precompute it and load it in a text file or something similar

Bitboard computeBishopAttacks(int square, Bitboard blockers) {
    Bitboard attacks = 0;
    for (int direction : BISHOP_DIRECTIONS) {
        int currentSquare = square;
        while (true) {
            int previousSquare = currentSquare;
            currentSquare += direction;
            if (currentSquare < 0 || currentSquare > 63) break;
            if (std::abs((currentSquare % 8) - (previousSquare % 8)) != 1) break;
            attacks |= (1ULL << currentSquare);
            if (blockers & (1ULL << currentSquare)) break;
        }
    }
    return attacks;
}

Bitboard computeRookAttacks(int square, Bitboard blockers) {
    Bitboard attacks = 0;
    for (int direction : ROOK_DIRECTIONS) {
        int currentSquare = square;
        while (true) {
            int previousSquare = currentSquare;
            currentSquare += direction;
            if (currentSquare < 0 || currentSquare > 63) break;
            if (std::abs(direction) == 1 && currentSquare / 8 != previousSquare / 8) break;
            attacks |= (1ULL << currentSquare);
            if (blockers & (1ULL << currentSquare)) break;
        }
    }
    return attacks;
}

bool testBishopMagic(int square, Bitboard magic) {
    Bitboard mask = BISHOP_MASK[square];
    int shift = BISHOP_SHIFT[square];

    Bitboard used[512] = {};
    Bitboard subset = 0;

    while (true) {
        Bitboard attacks = computeBishopAttacks(square, subset);
        int index = (subset * magic) >> shift;
        if (used[index] == 0) {
            used[index] = attacks;
        }
        else if (used[index] != attacks) {
            return false;
        }
        subset = (subset - mask) & mask;
        if (subset == 0) break;
    }

    subset = 0;
    while (true) {
        int index = (subset * magic) >> shift;
        BISHOP_ATTACKS[square][index] = computeBishopAttacks(square, subset);
        subset = (subset - mask) & mask;
        if (subset == 0) break;
    }
    return true;
}

bool testRookMagic(int square, Bitboard magic) {
    Bitboard mask = ROOK_MASK[square];
    int shift = ROOK_SHIFT[square];

    Bitboard used[4096] = {};
    Bitboard subset = 0;

    while (true) {
        Bitboard attacks = computeRookAttacks(square, subset);
        int index = (subset * magic) >> shift;
        if (used[index] == 0) {
            used[index] = attacks;
        }
        else if (used[index] != attacks) {
            return false;
        }
        subset = (subset - mask) & mask;
        if (subset == 0) break;
    }

    subset = 0;
    while (true) {
        int index = (subset * magic) >> shift;
        ROOK_ATTACKS[square][index] = computeRookAttacks(square, subset);
        subset = (subset - mask) & mask;
        if (subset == 0) break;
    }
    return true;
}

Bitboard findBishopMagic(int square) {
    while (true) {
        Bitboard magic = rng() & rng() & rng();
        if (testBishopMagic(square, magic)) return magic;
    }
}

Bitboard findRookMagic(int square) {
    while (true) {
        Bitboard magic = rng() & rng() & rng();
        if (testRookMagic(square, magic)) return magic;
    }
}

void magic::initMagicTables() {
    static bool initialized = false;
    if (initialized) return;
    initialized = true;

    // PAWN
    for (int square = 0; square < SQUARE_NB; square++) {
        Bitboard squareMask = 1ULL << square;
        W_PAWN_ATTACKS[square] = ((squareMask & NOT_FILE8) << 9) | ((squareMask & NOT_FILE1) << 7);
        B_PAWN_ATTACKS[square] = ((squareMask & NOT_FILE1) >> 9) | ((squareMask & NOT_FILE8) >> 7);
    }

    // KNIGHTS
    for (int square = 0; square < SQUARE_NB; square++) {
        Bitboard mask = 0;
        Bitboard squareMask = 1ULL << square;
        mask |= (squareMask & NOT_FILE8)             << 17;
        mask |= (squareMask & NOT_FILE8 & NOT_FILE7) << 10;
        mask |= (squareMask & NOT_FILE8 & NOT_FILE7) >> 6;
        mask |= (squareMask & NOT_FILE8)             >> 15;
        mask |= (squareMask & NOT_FILE1)             << 15;
        mask |= (squareMask & NOT_FILE1 & NOT_FILE2) << 6;
        mask |= (squareMask & NOT_FILE1 & NOT_FILE2) >> 10;
        mask |= (squareMask & NOT_FILE1)             >> 17;

        KNIGHT_ATTACKS[square] = mask;
    }

    // BISHOP
    for (int square = 0; square < SQUARE_NB; square++) {
        Bitboard mask = 0;
        for (int d = 0; d < 4; d++) {
            int direction = BISHOP_DIRECTIONS[d];
            int currentSquare = square;
            while (true) {
                int previousSquare = currentSquare;
                currentSquare += direction;
                if (currentSquare < 0 || currentSquare > 63) break;
                if (std::abs((currentSquare % 8) - (previousSquare % 8)) != 1) break;
                int rank = currentSquare / RANK_NB;
                int file = currentSquare % RANK_NB;
                if (rank == 0 || rank == 7 || file == 0 || file == 7) break;
                mask |= (1ULL << currentSquare);
            }
        }
        BISHOP_MASK[square] = mask;
        BISHOP_SHIFT[square] = 64 - __builtin_popcountll(mask);
        BISHOP_MAGIC[square] = findBishopMagic(square);
    }

    // ROOK
    for (int square = 0; square < SQUARE_NB; square++) {
        Bitboard mask = 0;
        for (int d = 0; d < 4; d++) {
            int direction = ROOK_DIRECTIONS[d];
            int currentSquare = square;
            while (true) {
                int previousSquare = currentSquare;
                currentSquare += direction;
                if (currentSquare < 0 || currentSquare > 63) break;
                if (std::abs(direction) == 1 && currentSquare / 8 != previousSquare / 8) break;
                int rank = currentSquare / RANK_NB;
                int file = currentSquare % RANK_NB;
                if (direction ==  8 && rank == 7) break;
                if (direction == -8 && rank == 0) break;
                if (direction ==  1 && file == 7) break;
                if (direction == -1 && file == 0) break;
                mask |= (1ULL << currentSquare);
            }
        }
        ROOK_MASK[square] = mask;
        ROOK_SHIFT[square] = 64 - __builtin_popcountll(mask);
        ROOK_MAGIC[square] = findRookMagic(square);
    }

    // KING
    for (int square = 0; square < SQUARE_NB; square++) {
        Bitboard mask = 0;
        Bitboard squareMask = 1ULL << square;
        mask |= (squareMask & NOT_FILE8) << 9; // up-right
        mask |= squareMask               << 8; // up
        mask |= (squareMask & NOT_FILE1) << 7; // up-left
        mask |= (squareMask & NOT_FILE8) << 1; // right
        mask |= (squareMask & NOT_FILE1) >> 1; // left
        mask |= (squareMask & NOT_FILE8) >> 7; // down-right
        mask |= squareMask               >> 8; // down
        mask |= (squareMask & NOT_FILE1) >> 9; // down-left

        KING_ATTACKS[square] = mask;
    }
}

Bitboard magic::getPawnAttacks(int square, int color) {
    return (color == WHITE) ? W_PAWN_ATTACKS[square] : B_PAWN_ATTACKS[square];
}

Bitboard magic::getKnightAttacks(int square) {
    return KNIGHT_ATTACKS[square];
}

Bitboard magic::getBishopAttacks(int square, Bitboard occupied) {
    Bitboard blockers = occupied & BISHOP_MASK[square];
    int index = (blockers * BISHOP_MAGIC[square]) >> BISHOP_SHIFT[square];
    return BISHOP_ATTACKS[square][index];
}

Bitboard magic::getRookAttacks(int square, Bitboard occupied) {
    Bitboard blockers = occupied & ROOK_MASK[square];
    int index = (blockers * ROOK_MAGIC[square]) >> ROOK_SHIFT[square];
    return ROOK_ATTACKS[square][index];
}

Bitboard magic::getQueenAttacks(int square, Bitboard occupied) {
    return magic::getBishopAttacks(square, occupied) | magic::getRookAttacks(square, occupied);
}

Bitboard magic::getKingAttacks(int square) {
    return KING_ATTACKS[square];
}
