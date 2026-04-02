#include <cstdint>

#include "evaluate.h"
#include "board.h"
#include "constants.h"
#include "pieceSquareTable.h"

// based on CPW-Engine eval
// https://www.chessprogramming.org/CPW-Engine_eval

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
    
}

void evalPieceActivity(const Board& board, int& mgScore, int& egScore) {
    Bitboard whiteBishops = board.byTypeBB[BISHOP] & board.byColorBB[WHITE];
    Bitboard blackBishops = board.byTypeBB[BISHOP] & board.byColorBB[BLACK];

    // reward the bishop pair
    if (__builtin_popcountll(whiteBishops) >= 2) {
        mgScore += 30;
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

int evaluate(const Board& board) {
    int mgScore = 0;
    int egScore = 0;

    evalMaterialAndPst(board, mgScore, egScore);
    evalPawnStructure(board, mgScore, egScore);
    evalPieceActivity(board, mgScore, egScore);
    evalKingSafety(board, mgScore, egScore);

    int score = computeStage(board, mgScore, egScore);
    return board.side == WHITE ? score : -score;
}