#include "board.h"

#include <cctype>
#include <iostream>
#include <sstream>

#include "evaluate.h"
#include "magic.h"
#include "utils.h"
#include "zobrist.h"

/*
TODO:
- fix the method isIncheck
*/

Board::Board() {
    zobrist::initialize();
    magic::initialize();
    eval::initialize();
    clear();
}

Board::Board(const std::string& fen) : Board() {
    loadFen(fen);
}

void Board::clear() {
    ply = 0;
    for (int i = 0; i < SQUARE_NB; i++) {
        squares[i] = NO_PIECE;
    }
    for (int piece = 0; piece < PIECE_TYPE_NB; piece++) {
        byTypeBB[piece] = 0;
    }

    for (int color = 0; color < COLOR_NB; color++) {
        byColorBB[color] = 0;
    }

    castlingRights = WHITE_OO | WHITE_OOO | BLACK_OO | BLACK_OOO;
    key = 0;
}

void Board::loadFen(const std::string& fen) {
    clear();
    std::istringstream ss(fen);
    std::string placement, activeColor, castling, epStr;
    ss >> placement >> activeColor >> castling >> epStr;

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
        PieceType pieceType = NO_PIECE_TYPE;
        Piece piece = NO_PIECE;
        if (c == 'P') {
            pieceType = PAWN;
            piece = W_PAWN;
        }
        else if (c == 'N') {
            pieceType = KNIGHT;
            piece = W_KNIGHT;
        }
        else if (c == 'B') {
            pieceType = BISHOP;
            piece = W_BISHOP;
        }
        else if (c == 'R') {
            pieceType = ROOK;
            piece = W_ROOK;
        }
        else if (c == 'Q') {
            pieceType = QUEEN;
            piece = W_QUEEN;
        }
        else if (c == 'K') {
            pieceType = KING;
            piece = W_KING;
        }
        else if (c == 'p') {
            pieceType = PAWN;
            piece = B_PAWN;
        }
        else if (c == 'n') {
            pieceType = KNIGHT;
            piece = B_KNIGHT;
        }
        else if (c == 'b') {
            pieceType = BISHOP;
            piece = B_BISHOP;
        }
        else if (c == 'r') {
            pieceType = ROOK;
            piece = B_ROOK;
        }
        else if (c == 'q') {
            pieceType = QUEEN;
            piece = B_QUEEN;
        }
        else if (c == 'k') {
            pieceType = KING;
            piece = B_KING;
        }

        squares[rank * 8 + file] = piece;

        byTypeBB[pieceType] |= bit;
        byColorBB[color] |= bit;        

        file++;
    }

    // side
    side = (activeColor == "w") ? WHITE : BLACK;

    // castling
    castlingRights = 0;
    if (castling.find('K') != std::string::npos) castlingRights |= WHITE_OO;
    if (castling.find('Q') != std::string::npos) castlingRights |= WHITE_OOO;
    if (castling.find('k') != std::string::npos) castlingRights |= BLACK_OO;
    if (castling.find('q') != std::string::npos) castlingRights |= BLACK_OOO;

    // en passant
    epSquare = NO_SQUARE;
    if (!epStr.empty() && epStr != "-") {
        int file = epStr[0] - 'a';
        int rank = epStr[1] - '1';
        epSquare = rank * 8 + file;
    }

    key = hash();
}

// move this to cout overload
void Board::display() const {
    const char* symbols[PIECE_NB] = {
        ".", 
        "♙", "♘", "♗", "♖", "♕", "♔", // WHITE
        ".", ".",
        "♟", "♞", "♝", "♜", "♛", "♚", // BLACK
    };

    for (int rank = 7; rank >= 0; rank--) {
        std::cout << rank + 1 << " ";
        for (int file = 0; file < 8; file++) {
            int square = rank * 8 + file;
            int piece = squares[square];
            const char* symbol = symbols[piece];
            std::cout << symbol << " ";
        }
        std::cout << "\n";
    }
    std::cout << "  a b c d e f g h\n";
}

void Board::switchSide() {
    side = static_cast<Color>(side ^ 1);
}

bool Board::isSquareAttacked(int square, int color) const {
    int enemyColor = color ^ 1;

    Bitboard occupied = byColorBB[WHITE] | byColorBB[BLACK];

    Bitboard pawns   = byTypeBB[PAWN] & byColorBB[color];
    Bitboard knights = byTypeBB[KNIGHT] & byColorBB[color];
    Bitboard bishops = byTypeBB[BISHOP] & byColorBB[color];
    Bitboard rooks   = byTypeBB[ROOK] & byColorBB[color];
    Bitboard queens  = byTypeBB[QUEEN] & byColorBB[color];
    Bitboard king    = byTypeBB[KING] & byColorBB[color];

    // by a pawns
    if (magic::getPawnAttacks(square, enemyColor) & pawns) {
        return true;
    }

    // knights
    if (magic::getKnightAttacks(square) & knights) {
        return true;
    }

    // diagonals (bishops & queens)
    if (magic::getBishopAttacks(square, occupied) & (bishops | queens)) {
        return true;
    }

    // straight lines (rooks & queens)
    if (magic::getRookAttacks(square, occupied) & (rooks | queens)) {
        return true;
    }

    // king
    if (magic::getKingAttacks(square) & king) return true;

    // no pieces attack the square
    return false;
};

bool Board::isInCheck(int color) const {
    Bitboard king = byTypeBB[KING] & byColorBB[color];
    int kingSquare = __builtin_ctzll(king);
    return isSquareAttacked(kingSquare, color ^ 1);
}

Key Board::hash() const {
    Key key = 0;
    for (int square = 0; square < SQUARE_NB; square++) {
        int piece = squares[square];
        if (piece == NO_PIECE) continue;
        zobrist::updateKeyPiece(key, piece, square);
    }

    if (side == BLACK) {
        zobrist::updateKeySide(key);
    }

    zobrist::updateKeyCastle(key, castlingRights);

    if (epSquare != NO_SQUARE) {
        zobrist::updateKeyEp(key, epSquare);
    }

    return key;
}
