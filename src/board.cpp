#include <iostream>
#include <sstream>
#include <cctype>
#include <random>

#include "board.h"
#include "magic.h"

namespace zobrist {

    uint64_t pieceKeys[PIECES][COLORS][SQUARES];
    uint64_t sideKey;
    uint64_t castleKeys[16];
    uint64_t enPassantKeys[SQUARES];

    bool zobristInitialized = false;

    void initZobrist() {
        if (zobristInitialized) return;

        std::mt19937_64 rng(57);

        for (int piece = PAWN; piece <= KING; piece++) {
            for (int color = 0; color < COLORS; color++) {
                for (int square = 0; square < SQUARES; square++) {
                    pieceKeys[piece][color][square] = rng();
                }   
            }
        }

        sideKey = rng();
        for (int d = 0; d < 16; d++) {
            castleKeys[d] = rng();
        }

        for (int square = 0; square < SQUARES; square++) {
            enPassantKeys[square] = rng();
        }

        zobristInitialized = true;
}

    void updateKeyPiece(Key& key, int piece, int color, int square) {
        key ^= pieceKeys[piece][color][square];
    }

    void updateKeyside(Key& key) {
        key ^= sideKey;
    }

    void updateKeyCastle(Key& key, int castle) {
        key ^= castleKeys[castle];
    }

    void updateKeyEnPassant(Key& key, int square) {
        key ^= enPassantKeys[square];
    }
}

Board::Board(std::string fen) {
    zobrist::initZobrist();
    std::istringstream ss(fen);
    std::string placement, activeColor, castling, enPassantStr;
    ss >> placement >> activeColor >> castling >> enPassantStr;

    for (int i = 0; i < SQUARES; i++) {
        pieceBoard[i] = NO_PIECE;
    }

    // parse piece placement
    int rank = 7, file = 0;
    for (char c : placement) {
        if (c == '/') {
            rank--;
            file = 0;
            continue;
        }
        if (std::isdigit(c)) {
            file += c - '0';
            continue;
        }
        int color = std::islower(c) ? BLACK : WHITE;
        Bitboard bit = 1ULL << (rank * 8 + file);
        char lowCase = std::tolower(c);
        int pieceType = NO_PIECE;
        if (lowCase == 'p') {
            bitboards[PAWN][color] |= bit;
            pieceType = PAWN;
        }
        if (lowCase == 'n') {
            bitboards[KNIGHT][color] |= bit;
            pieceType = KNIGHT;
        }
        if (lowCase == 'b') {
            bitboards[BISHOP][color] |= bit;
            pieceType = BISHOP;
        }
        if (lowCase == 'r') {
            bitboards[ROOK][color]   |= bit;
            pieceType = ROOK;
        }
        if (lowCase == 'q') {
            bitboards[QUEEN][color]  |= bit;
            pieceType = QUEEN;
        }
        if (lowCase == 'k') {
            bitboards[KING][color]   |= bit;
            pieceType = KING;
        }
        pieceBoard[rank * 8 + file] = pieceType;
        wbPieces[color] |= bit;

        file++;
    }

    occupied = wbPieces[0] | wbPieces[1];

    // turn
    turn = (activeColor == "w") ? WHITE : BLACK;

    // castling
    kingCastle[WHITE]  = castling.find('K') != std::string::npos;
    queenCastle[WHITE] = castling.find('Q') != std::string::npos;
    kingCastle[BLACK]  = castling.find('k') != std::string::npos;
    queenCastle[BLACK] = castling.find('q') != std::string::npos;

    // en passant
    enPassant = (!enPassantStr.empty() && enPassantStr != "-");
    if (enPassant) {
        int file = enPassantStr[0] - 'a';
        int rank = enPassantStr[1] - '1';
        enPassantSquare = rank * 8 + file;
    }

    key = hash();
}

void Board::display() const {
    const char* symbols[2][7] = {
        {".", "♙", "♘", "♗", "♖", "♕", "♔"},  // WHITE
        {".", "♟", "♞", "♝", "♜", "♛", "♚"},  // BLACK
    };

    for (int rank = 7; rank >= 0; rank--) {
        std::cout << rank + 1 << " ";
        for (int file = 0; file < 8; file++) {
            int square = rank * 8 + file;
            const char* symbol = ".";
            for (int color = 0; color < 2; color++) {
                for (int piece = PAWN; piece <= KING; piece++) {
                    if (bitboards[piece][color] & (1LL << square)) {
                        symbol = symbols[color][piece];
                    }
                }
            }
            std::cout << symbol << " ";
        }
        std::cout << "\n";
    }
    std::cout << "  a b c d e f g h\n";
}

void Board::nextTurn() {
    turn ^= 1;
}

bool Board::isSquareAttacked(int square, int color) const {
    int enemyColor = color ^ 1;
    Bitboard squareMask = 1ULL << square;

    Bitboard pawns = bitboards[PAWN][color];
    Bitboard knights = bitboards[KNIGHT][color];
    Bitboard bishops = bitboards[BISHOP][color];
    Bitboard rooks = bitboards[ROOK][color];
    Bitboard queens = bitboards[QUEEN][color];
    Bitboard king = bitboards[KING][color];

    // by a pawns
    Bitboard attackers = 0;
    if (color == WHITE) {
        attackers = (((squareMask & NOT_FILE1) >> 9) | ((squareMask & NOT_FILE8) >> 7)) & pawns;
    }
    if (color == BLACK) {
        attackers = (((squareMask & NOT_FILE8) << 9) | ((squareMask & NOT_FILE1) << 7)) & pawns;
    }
    if (attackers) {
        return true;
    }

    // knights
    attackers |= (squareMask & NOT_FILE8)             << 17;
    attackers |= (squareMask & NOT_FILE8 & NOT_FILE7) << 10;
    attackers |= (squareMask & NOT_FILE8 & NOT_FILE7) >> 6;
    attackers |= (squareMask & NOT_FILE8)             >> 15;
    attackers |= (squareMask & NOT_FILE1)             << 15;
    attackers |= (squareMask & NOT_FILE1 & NOT_FILE2) << 6;
    attackers |= (squareMask & NOT_FILE1 & NOT_FILE2) >> 10;
    attackers |= (squareMask & NOT_FILE1)             >> 17;
    if (attackers & knights) {
        return true;
    }

    // diagonals (bishops & queens)
    for (int direction : {9, 7, -7, -9}) {
        int currentSquare = square;
        while (true) {
            int previousSquare = currentSquare;
            currentSquare += direction;
            // TODO: wrap in a function outOfBoard
            if (currentSquare < 0 || currentSquare > 63) {
                break;
            }
            if (std::abs(currentSquare % 8 - previousSquare % 8) != 1) {
                break;
            }
            Bitboard target = 1ULL << currentSquare;
            if (target & (bishops | queens)) {
                return true;
            }
            if (target & occupied) {
                break;
            }
        }
    }

    // straight lines (rooks & queens)
    for (int direction : {8, -8, 1, -1}) {
        int currentSquare = square;
        while (true) {
            int previousSquare = currentSquare;
            currentSquare += direction;
            // TODO: wrap in a function outOfBoard
            if (currentSquare < 0 || currentSquare > 63) {
                break;
            }
            if (std::abs(direction) == 1 && currentSquare / 8 != previousSquare / 8) {
                break;
            }
            Bitboard target = 1ULL << currentSquare;
            if (target & (rooks | queens)) {
                return true;
            }
            if (target & occupied) {
                break;
            }
        }
    }

    // king
    attackers = 0;
    attackers |= (squareMask & NOT_FILE8) << 9;
    attackers |= squareMask               << 8;
    attackers |= (squareMask & NOT_FILE1) << 7;
    attackers |= (squareMask & NOT_FILE8) << 1;
    attackers |= (squareMask & NOT_FILE1) >> 1;
    attackers |= (squareMask & NOT_FILE8) >> 7;
    attackers |= squareMask               >> 8;
    attackers |= (squareMask & NOT_FILE1) >> 9;

    if (attackers & king) {
        return true;
    }

    // no pieces attack the square
    return false;
};

bool Board::isInCheck(int color) const {
    int kingSquare = __builtin_ctzll(bitboards[KING][color]);
    return isSquareAttacked(kingSquare, color ^ 1);
}

Key Board::hash() const {
    Key key = 0;
    for (int square = 0; square < SQUARES; square++) {
        int piece = pieceBoard[square];
        if (piece == NO_PIECE) continue;

        int color = WHITE;
        if (wbPieces[BLACK] & (1ULL << square)) {
            color = BLACK;
        }

        key ^= zobrist::pieceKeys[piece][color][square];
    }

    if (turn == BLACK) {
        key ^= zobrist::sideKey;
    }

    int castle = 0;
    if (kingCastle[WHITE])  castle |= 1;
    if (queenCastle[WHITE]) castle |= 2;
    if (kingCastle[BLACK])  castle |= 4;
    if (queenCastle[BLACK]) castle |= 8;

    key ^= zobrist::castleKeys[castle];

    if (enPassant) {
        key ^= zobrist::enPassantKeys[enPassantSquare];
    }

    return key;
}
