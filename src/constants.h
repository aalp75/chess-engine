#pragma once

#include <cstdint>

using Bitboard = uint64_t;
using Move     = uint32_t;
using Key      = uint64_t;

constexpr int SQUARE_NB = 64;

constexpr int RANK_NB = 8;
constexpr int FILE_NB = 8;

enum Color { 
    WHITE, BLACK,
    COLOR_NB = 2
};

constexpr int NO_SQUARE = -1;

enum PieceType {
    NO_PIECE_TYPE = 0,
    PAWN = 1, KNIGHT, BISHOP, ROOK, QUEEN, KING,
    PIECE_TYPE_NB = 7
};

enum Piece { 
    NO_PIECE, 
    W_PAWN = 1, W_KNIGHT, W_BISHOP, W_ROOK, W_QUEEN, W_KING,
    B_PAWN = 9, B_KNIGHT, B_BISHOP, B_ROOK, B_QUEEN, B_KING,
    PIECE_NB = 16
};

enum CastlingRights : uint8_t {
    WHITE_OO  = 1 << 0,  // 0001
    WHITE_OOO = 1 << 1,  // 0010
    BLACK_OO  = 1 << 2,  // 0100
    BLACK_OOO = 1 << 3   // 1000
};

enum MoveType { NORMAL, EN_PASSANT, CASTLING, PROMOTION };

constexpr int INF = 100'000;
constexpr int MATE_SCORE = INF;
constexpr int MATE_BOUND = INF - 100;

constexpr int KILLER_SCORE1 = INF - 1'000;
constexpr int KILLER_SCORE2 = INF - 2'000;

constexpr int QS_DEPTH = -1;

constexpr int DELTA_MARGIN = 200;

constexpr int BISHOP_DIRECTIONS[4] = {9, 7, -7, -9};
constexpr int ROOK_DIRECTIONS[4] = {8, 1, -1, -8};
constexpr int QUEEN_DIRECTIONS[8] = {8, 1, -1, -8, 9, 7, -7, -9};

constexpr uint64_t RANK1 = 0x00000000000000FFULL;
constexpr uint64_t RANK2 = 0x000000000000FF00ULL;
constexpr uint64_t RANK3 = 0x0000000000FF0000ULL;
constexpr uint64_t RANK4 = 0x00000000FF000000ULL;
constexpr uint64_t RANK5 = 0x000000FF00000000ULL;
constexpr uint64_t RANK6 = 0x0000FF0000000000ULL;
constexpr uint64_t RANK7 = 0x00FF000000000000ULL;
constexpr uint64_t RANK8 = 0xFF00000000000000ULL;

constexpr uint64_t FILE1 = 0x0101010101010101ULL; // a file
constexpr uint64_t FILE2 = 0x0202020202020202ULL; // b file
constexpr uint64_t FILE3 = 0x0404040404040404ULL; // c file
constexpr uint64_t FILE4 = 0x0808080808080808ULL; // d file
constexpr uint64_t FILE5 = 0x1010101010101010ULL; // e file
constexpr uint64_t FILE6 = 0x2020202020202020ULL; // f file
constexpr uint64_t FILE7 = 0x4040404040404040ULL; // g file
constexpr uint64_t FILE8 = 0x8080808080808080ULL; // h file

constexpr uint64_t NOT_RANK1 = ~RANK1;
constexpr uint64_t NOT_RANK2 = ~RANK2;
constexpr uint64_t NOT_RANK3 = ~RANK3;
constexpr uint64_t NOT_RANK4 = ~RANK4;
constexpr uint64_t NOT_RANK5 = ~RANK5;
constexpr uint64_t NOT_RANK6 = ~RANK6;
constexpr uint64_t NOT_RANK7 = ~RANK7;
constexpr uint64_t NOT_RANK8 = ~RANK8;

constexpr uint64_t NOT_FILE1 = ~FILE1;
constexpr uint64_t NOT_FILE2 = ~FILE2;
constexpr uint64_t NOT_FILE3 = ~FILE3;
constexpr uint64_t NOT_FILE4 = ~FILE4;
constexpr uint64_t NOT_FILE5 = ~FILE5;
constexpr uint64_t NOT_FILE6 = ~FILE6;
constexpr uint64_t NOT_FILE7 = ~FILE7;
constexpr uint64_t NOT_FILE8 = ~FILE8;

constexpr const char* FEN_START =
    "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";


