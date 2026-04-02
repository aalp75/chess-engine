#include<vector>

#include "moves.h"
#include "constants.h"
#include "board.h"
#include "moveList.h"
#include "magic.h"

/*

TODO: 
- clean a bit the Zobrist incremental update
- clean a bit the move generation (more generic)

type:

NORMAL     = 0
EN_PASSANT = 1
CASTLING   = 2
PROMOTION  = 3

Note: __builtin_ctzll count trailing zeros in the binary representation
__builtin_ctzll(12) = 2 because 12 = 1100 --> move this to a helper

*/

void doMove(Board& board, Move move, StateInfo* states, int ply) {
    int fromSquare = moveFrom(move);
    int toSquare   = moveTo(move);
    int type       = moveType(move);
    int promoPiece = movePromo(move);

    int color = board.turn;
    int enemyColor = color ^ 1;

    Piece piece         = board.squares[fromSquare];
    PieceType pieceType = PieceType(piece & 7);
    Piece captured      = (type == EN_PASSANT) ? Piece(PAWN | (enemyColor << 3)) : board.squares[toSquare];

    StateInfo& newState = states[ply];

    newState.fromSquare     = fromSquare;
    newState.toSquare       = toSquare;
    newState.movedPiece     = piece;
    newState.capturedPiece  = captured;
    newState.kingCastle[0]  = board.kingCastle[0]; 
    newState.kingCastle[1]  = board.kingCastle[1];
    newState.queenCastle[0] = board.queenCastle[0];
    newState.queenCastle[1] = board.queenCastle[1];
    newState.type           = type;
    newState.epSquare       = board.epSquare;
    newState.promoPiece     = promoPiece;
    newState.key            = board.key;

    int oldCastle = (board.kingCastle[WHITE]  ? 1 : 0) | (board.queenCastle[WHITE] ? 2 : 0)
                  | (board.kingCastle[BLACK]  ? 4 : 0) | (board.queenCastle[BLACK] ? 8 : 0);

    zobrist::updateKeyCastle(board.key, oldCastle);

    if (board.epSquare != NO_SQUARE) {
        zobrist::updateKeyEp(board.key, board.epSquare);
    }

    board.epSquare = NO_SQUARE;

    zobrist::updateKeyPiece(board.key, piece, fromSquare);
    Piece landingPiece = (type == PROMOTION) ? Piece(promoPiece | (color << 3)) : piece;
    zobrist::updateKeyPiece(board.key, landingPiece, toSquare);

    if (captured != NO_PIECE && type != EN_PASSANT)
        zobrist::updateKeyPiece(board.key, captured, toSquare);

    // set en passant square for double pawn push
    if (pieceType == PAWN && std::abs(toSquare - fromSquare) == 16) {
        board.epSquare = (fromSquare + toSquare) / 2;
        zobrist::updateKeyEp(board.key, board.epSquare);
    }

    if (captured != NO_PIECE) {
        board.byTypeBB[captured & 7] &= ~(1ULL << toSquare);
        board.byColorBB[enemyColor]  &= ~(1ULL << toSquare);
    }

    board.byTypeBB[pieceType] &= ~(1ULL << fromSquare);
    board.byTypeBB[pieceType] |=  (1ULL << toSquare);

    board.byColorBB[color] &= ~(1ULL << fromSquare);
    board.byColorBB[color] |=  (1ULL << toSquare);

    board.squares[fromSquare] = NO_PIECE;
    board.squares[toSquare]   = piece;

    // if a rook is captured remove castle
    if (toSquare == 0) {
        board.queenCastle[WHITE] = false;
    }
    if (toSquare == 7) {
        board.kingCastle[WHITE] = false;
    }
    if (toSquare == 56) {
        board.queenCastle[BLACK] = false;
    }
    if (toSquare == 63) {
        board.kingCastle[BLACK] = false;
    }

    // en passant
    if (type == EN_PASSANT) {
        int capturedSquare = (color == WHITE) ? toSquare - 8 : toSquare + 8;
        
        board.byTypeBB[PAWN] &= ~ (1ULL << capturedSquare);
        board.byColorBB[enemyColor] &= ~(1ULL << capturedSquare);

        board.squares[capturedSquare] = NO_PIECE;

        newState.capturedSquare = capturedSquare;
        zobrist::updateKeyPiece(board.key, Piece(PAWN | (enemyColor << 3)), capturedSquare);
    }

    // update castle if a king or a rook move
    if (fromSquare == 4) {
        board.kingCastle[WHITE] = false;
        board.queenCastle[WHITE] = false;
    }
    if (fromSquare == 60) {
        board.kingCastle[BLACK] = false;
        board.queenCastle[BLACK] = false;
    }
    if (fromSquare == 0) {
        board.queenCastle[WHITE] = false;
    }
    if (fromSquare == 7) {
        board.kingCastle[WHITE] = false;
    }
    if (fromSquare == 56) {
        board.queenCastle[BLACK] = false;
    }
    if (fromSquare == 63) {
        board.kingCastle[BLACK] = false;
    }

    int newCastle = (board.kingCastle[WHITE]  ? 1 : 0) | (board.queenCastle[WHITE] ? 2 : 0)
                    | (board.kingCastle[BLACK]  ? 4 : 0) | (board.queenCastle[BLACK] ? 8 : 0);
    zobrist::updateKeyCastle(board.key, newCastle);

    // promotion
    if (type == PROMOTION) {
        board.byTypeBB[PAWN]       &= ~(1ULL << toSquare);
        board.byTypeBB[promoPiece] |= (1ULL << toSquare);
        
        board.squares[toSquare] = Piece(promoPiece | (color << 3));
        newState.promoPiece = promoPiece;
    }

    if (type == CASTLING) {
        
        if (toSquare == 6) { // white O-O
            board.byTypeBB[ROOK]   &= ~(1ULL << 7);
            board.byTypeBB[ROOK]   |=  (1ULL << 5);
            board.byColorBB[WHITE] &= ~(1ULL << 7);
            board.byColorBB[WHITE] |=  (1ULL << 5);
            board.squares[7] = NO_PIECE;
            board.squares[5] = W_ROOK;
            zobrist::updateKeyPiece(board.key, W_ROOK, 7);
            zobrist::updateKeyPiece(board.key, W_ROOK, 5);
        }
        else if (toSquare == 2) { // white O-O-O
            board.byTypeBB[ROOK]   &= ~(1ULL << 0);
            board.byTypeBB[ROOK]   |=  (1ULL << 3);
            board.byColorBB[WHITE] &= ~(1ULL << 0);
            board.byColorBB[WHITE] |=  (1ULL << 3);
            board.squares[0] = NO_PIECE;
            board.squares[3] = W_ROOK;
            zobrist::updateKeyPiece(board.key, W_ROOK, 0);
            zobrist::updateKeyPiece(board.key, W_ROOK, 3);
        }
        else if (toSquare == 62) { // black O-O
            board.byTypeBB[ROOK]   &= ~(1ULL << 63);
            board.byTypeBB[ROOK]   |=  (1ULL << 61);
            board.byColorBB[BLACK] &= ~(1ULL << 63);
            board.byColorBB[BLACK] |=  (1ULL << 61);
            board.squares[63] = NO_PIECE;
            board.squares[61] = B_ROOK;
            zobrist::updateKeyPiece(board.key, B_ROOK, 63);
            zobrist::updateKeyPiece(board.key, B_ROOK, 61);
        }
        else if (toSquare == 58) { // black O-O-O
            board.byTypeBB[ROOK]   &= ~(1ULL << 56);
            board.byTypeBB[ROOK]   |=  (1ULL << 59);
            board.byColorBB[BLACK] &= ~(1ULL << 56);
            board.byColorBB[BLACK] |=  (1ULL << 59);
            board.squares[56] = NO_PIECE;
            board.squares[59] = B_ROOK;
            zobrist::updateKeyPiece(board.key, B_ROOK, 56);
            zobrist::updateKeyPiece(board.key, B_ROOK, 59);
        }
    }

    board.switchTurn();
    zobrist::updateKeySide(board.key);
}

void undoMove(Board& board, StateInfo* states, int ply) {

    StateInfo& state = states[ply];

    int fromSquare      = state.fromSquare;
    int toSquare        = state.toSquare;
    Piece piece         = Piece(state.movedPiece);
    PieceType pieceType = PieceType(piece & 7);
    Piece captured      = Piece(state.capturedPiece);
    int color           = board.turn ^ 1;
    int enemyColor      = color ^ 1;
    int promoPiece      = state.promoPiece;

    if (state.type == PROMOTION) {
        board.byTypeBB[promoPiece] &= ~(1ULL << toSquare);
    }

    // move piece back
    board.byTypeBB[pieceType] &= ~(1ULL << toSquare);
    board.byTypeBB[pieceType] |=  (1ULL << fromSquare);
    board.byColorBB[color]    &= ~(1ULL << toSquare);
    board.byColorBB[color]    |=  (1ULL << fromSquare);
    board.squares[toSquare]     = NO_PIECE;
    board.squares[fromSquare]   = piece;

    // restore captured piece (not for en passant, handled separately below)
    if (captured != NO_PIECE && state.type != EN_PASSANT) {
        board.byTypeBB[captured & 7] |= (1ULL << toSquare);
        board.byColorBB[enemyColor]  |= (1ULL << toSquare);
        board.squares[toSquare]         = captured;
    }

    // undo en passant
    if (state.type == EN_PASSANT) {
        int capturedSquare = state.capturedSquare;
        Piece enemyPawn = Piece(PAWN | (enemyColor << 3));
        board.byTypeBB[PAWN]        |= (1ULL << capturedSquare);
        board.byColorBB[enemyColor] |= (1ULL << capturedSquare);
        board.squares[capturedSquare]  = enemyPawn;
    }

    // undo castling (move rook back)
    if (state.type == CASTLING) {
        if (toSquare == 6) {
            board.byTypeBB[ROOK]   &= ~(1ULL << 5);
            board.byTypeBB[ROOK]   |=  (1ULL << 7);
            board.byColorBB[WHITE] &= ~(1ULL << 5);
            board.byColorBB[WHITE] |=  (1ULL << 7);
            board.squares[5] = NO_PIECE;
            board.squares[7] = W_ROOK;
        } else if (toSquare == 2) {
            board.byTypeBB[ROOK]   &= ~(1ULL << 3);
            board.byTypeBB[ROOK]   |=  (1ULL << 0);
            board.byColorBB[WHITE] &= ~(1ULL << 3);
            board.byColorBB[WHITE] |=  (1ULL << 0);
            board.squares[3] = NO_PIECE;
            board.squares[0] = W_ROOK;
        } else if (toSquare == 62) {
            board.byTypeBB[ROOK]   &= ~(1ULL << 61);
            board.byTypeBB[ROOK]   |=  (1ULL << 63);
            board.byColorBB[BLACK] &= ~(1ULL << 61);
            board.byColorBB[BLACK] |=  (1ULL << 63);
            board.squares[61] = NO_PIECE;
            board.squares[63] = B_ROOK;
        } else if (toSquare == 58) {
            board.byTypeBB[ROOK]   &= ~(1ULL << 59);
            board.byTypeBB[ROOK]   |=  (1ULL << 56);
            board.byColorBB[BLACK] &= ~(1ULL << 59);
            board.byColorBB[BLACK] |=  (1ULL << 56);
            board.squares[59] = NO_PIECE;
            board.squares[56] = B_ROOK;
        }
    }

    // restore board state
    board.kingCastle[0]  = state.kingCastle[0];
    board.kingCastle[1]  = state.kingCastle[1];
    board.queenCastle[0] = state.queenCastle[0];
    board.queenCastle[1] = state.queenCastle[1];
    board.epSquare       = state.epSquare;
    board.key            = state.key;
    board.turn           = board.turn ^ 1;
}

void doNullMove(Board& board, StateInfo* states, int ply) {
    StateInfo& newState = states[ply];
    newState.epSquare   = board.epSquare;
    newState.key        = board.key;

    if (board.epSquare != NO_SQUARE) {
        zobrist::updateKeyEp(board.key, board.epSquare);
    }

    board.epSquare = NO_SQUARE;
    zobrist::updateKeySide(board.key);
    board.switchTurn();
}

void undoNullMove(Board& board, StateInfo* states, int ply) {
    StateInfo& state = states[ply];
    board.key      = state.key;
    board.epSquare = state.epSquare;
    board.turn     = board.turn ^ 1;
}

MoveList generateMoves(const Board& board) {
    MoveList moves;

    generatePawnMoves(board, moves);
    generateKnightMoves(board, moves);
    generateBishopMoves(board, moves);
    generateRookMoves(board, moves);
    generateQueenMoves(board, moves);
    generateKingMoves(board, moves);

    return moves;
}

void generatePawnMoves(const Board& board, MoveList& moves) {
    int color = board.turn;
    
    Bitboard empty = ~(board.byColorBB[WHITE] | board.byColorBB[BLACK]);
    Bitboard enemies = board.byColorBB[color ^ 1];

    Bitboard pawns = board.byTypeBB[PAWN] & board.byColorBB[color];
    
    // single push
    if (color == WHITE) {
        Bitboard targets = (pawns << 8) & empty & NOT_RANK8;

        while (targets) {
            int toSquare = __builtin_ctzll(targets);
            int fromSquare = toSquare - 8;
            moves.addMove(fromSquare, toSquare, NORMAL);
            targets &= targets - 1;
        }
    }

    if (color == BLACK) {
        Bitboard targets = (pawns >> 8) & empty & NOT_RANK1;

        while (targets) {
            int toSquare = __builtin_ctzll(targets);
            int fromSquare = toSquare + 8;
            moves.addMove(fromSquare, toSquare, NORMAL);
            targets &= targets - 1;
        }
    }

    // double push
    if (color == WHITE) {
        Bitboard targets = ((pawns & RANK2) << 16) & empty & (empty << 8);

        while (targets) {
            int toSquare = __builtin_ctzll(targets);
            int fromSquare = toSquare - 16;
            moves.addMove(fromSquare, toSquare, NORMAL);
            targets &= targets - 1;
        }
    }

    if (color == BLACK) {
        Bitboard targets = ((pawns & RANK7) >> 16) & empty & (empty >> 8);

        while (targets) {
            int toSquare = __builtin_ctzll(targets);
            int fromSquare = toSquare + 16;
            moves.addMove(fromSquare, toSquare, NORMAL);
            targets &= targets - 1;
        }
    }

    // captures left / right
    if (color == WHITE) {
        // captures right (exclude rank 8, it's handled by capture promotions)
        Bitboard targets = ((pawns & NOT_FILE8) << 9) & enemies & NOT_RANK8;

        while (targets) {
            int toSquare = __builtin_ctzll(targets);
            int fromSquare = toSquare - 9;
            moves.addMove(fromSquare, toSquare, NORMAL);
            targets &= targets - 1;
        }

        // captures left (exclude rank 8, it's handled by capture promotions)
        targets = ((pawns & NOT_FILE1) << 7) & enemies & NOT_RANK8;

        while (targets) {
            int toSquare = __builtin_ctzll(targets);
            int fromSquare = toSquare - 7;
            moves.addMove(fromSquare, toSquare, NORMAL);
            targets &= targets - 1;
        }
    }

    if (color == BLACK) {
        // captures right (exclude rank 1, it's handled by capture promotions)
        Bitboard targets = ((pawns & NOT_FILE8) >> 7) & enemies & NOT_RANK1;

        while (targets) {
            int toSquare = __builtin_ctzll(targets);
            int fromSquare = toSquare + 7;
            moves.addMove(fromSquare, toSquare, NORMAL);
            targets &= targets - 1;
        }

        // captures left (exclude rank 1, it's handled by capture promotions)
        targets = ((pawns & NOT_FILE1) >> 9) & enemies & NOT_RANK1;

        while (targets) {
            int toSquare = __builtin_ctzll(targets);
            int fromSquare = toSquare + 9;
            moves.addMove(fromSquare, toSquare, NORMAL);
            targets &= targets - 1;
        }
    }

    // en passant
    if (board.epSquare != NO_SQUARE) {
        Bitboard ep = 1ULL << board.epSquare;
        Bitboard attackers = 0;
        if (color == WHITE) {
            attackers = (((ep & NOT_FILE1) >> 9) | ((ep & NOT_FILE8) >> 7)) & pawns;
        }
        if (color == BLACK) {
            attackers = (((ep & NOT_FILE8) << 9) | ((ep & NOT_FILE1) << 7)) & pawns;
        }

        while (attackers) {
            int fromSquare = __builtin_ctzll(attackers);
            moves.addMove(fromSquare, board.epSquare, EN_PASSANT, 0);
            attackers &= attackers - 1;
        }
    }

    // promotions
    if (color == WHITE) {
        Bitboard targets = (pawns << 8) & empty & RANK8;

        while (targets) {
            int toSquare = __builtin_ctzll(targets);
            int fromSquare = toSquare - 8;
            moves.addMove(fromSquare, toSquare, PROMOTION, QUEEN);
            moves.addMove(fromSquare, toSquare, PROMOTION, KNIGHT);
            moves.addMove(fromSquare, toSquare, PROMOTION, ROOK);
            moves.addMove(fromSquare, toSquare, PROMOTION, BISHOP);
            targets &= targets - 1;
        }
    }

    if (color == BLACK) {
        Bitboard targets = (pawns >> 8) & empty & RANK1;

        while (targets) {
            int toSquare = __builtin_ctzll(targets);
            int fromSquare = toSquare + 8;
            moves.addMove(fromSquare, toSquare, PROMOTION, QUEEN);
            moves.addMove(fromSquare, toSquare, PROMOTION, KNIGHT);
            moves.addMove(fromSquare, toSquare, PROMOTION, ROOK);
            moves.addMove(fromSquare, toSquare, PROMOTION, BISHOP);
            targets &= targets - 1;
        }
    }

    // capture promotions
    if (color == WHITE) {
        // capture right + promote
        Bitboard targets = ((pawns & NOT_FILE8) << 9) & enemies & RANK8;

        while (targets) {
            int toSquare = __builtin_ctzll(targets);
            int fromSquare = toSquare - 9;
            moves.addMove(fromSquare, toSquare, PROMOTION, QUEEN);
            moves.addMove(fromSquare, toSquare, PROMOTION, ROOK);
            moves.addMove(fromSquare, toSquare, PROMOTION, BISHOP);
            moves.addMove(fromSquare, toSquare, PROMOTION, KNIGHT);
            targets &= targets - 1;
        }

        // capture left + promote
        targets = ((pawns & NOT_FILE1) << 7) & enemies & RANK8;

        while (targets) {
            int toSquare = __builtin_ctzll(targets);
            int fromSquare = toSquare - 7;
            moves.addMove(fromSquare, toSquare, PROMOTION, QUEEN);
            moves.addMove(fromSquare, toSquare, PROMOTION, ROOK);
            moves.addMove(fromSquare, toSquare, PROMOTION, BISHOP);
            moves.addMove(fromSquare, toSquare, PROMOTION, KNIGHT);
            targets &= targets - 1;
        }
    }

    if (color == BLACK) {
        // capture right + promote
        Bitboard targets = ((pawns & NOT_FILE8) >> 7) & enemies & RANK1;

        while (targets) {
            int toSquare = __builtin_ctzll(targets);
            int fromSquare = toSquare + 7;
            moves.addMove(fromSquare, toSquare, PROMOTION, QUEEN);
            moves.addMove(fromSquare, toSquare, PROMOTION, ROOK);
            moves.addMove(fromSquare, toSquare, PROMOTION, BISHOP);
            moves.addMove(fromSquare, toSquare, PROMOTION, KNIGHT);
            targets &= targets - 1;
        }

        // capture left + promote
        targets = ((pawns & NOT_FILE1) >> 9) & enemies & RANK1;

        while (targets) {
            int toSquare = __builtin_ctzll(targets);
            int fromSquare = toSquare + 9;
            moves.addMove(fromSquare, toSquare, PROMOTION, QUEEN);
            moves.addMove(fromSquare, toSquare, PROMOTION, ROOK);
            moves.addMove(fromSquare, toSquare, PROMOTION, BISHOP);
            moves.addMove(fromSquare, toSquare, PROMOTION, KNIGHT);
            targets &= targets - 1;
        }
    }
}

void generateKnightMoves(const Board& board, MoveList& moves) {
    int color = board.turn;
    Bitboard allies = board.byColorBB[color];
    Bitboard knights = board.byTypeBB[KNIGHT] & board.byColorBB[color];

    while (knights) {
        int fromSquare = __builtin_ctzll(knights);
        knights &= knights - 1;

        Bitboard targets = magic::getKnightAttacks(fromSquare) & ~allies;

        while (targets) {
            int toSquare = __builtin_ctzll(targets);
            moves.addMove(fromSquare, toSquare, NORMAL, 0);
            targets &= targets - 1;
        }
    }
}

void generateBishopMoves(const Board& board, MoveList& moves) {
    int color = board.turn;
    Bitboard allies = board.byColorBB[color];
    Bitboard bishops = board.byTypeBB[BISHOP] & board.byColorBB[color];

    Bitboard occupied = board.byColorBB[WHITE] | board.byColorBB[BLACK];

    while (bishops) {
        int fromSquare = __builtin_ctzll(bishops);
        bishops &= bishops - 1;

        Bitboard attacks = magic::getBishopAttacks(fromSquare, occupied) & ~allies;

        while (attacks) {
            int toSquare = __builtin_ctzll(attacks);
            moves.addMove(fromSquare, toSquare, NORMAL, 0);
            attacks &= attacks - 1;
        }
    }
}

void generateRookMoves(const Board& board, MoveList& moves) {
    int color = board.turn;
    Bitboard allies = board.byColorBB[color];
    Bitboard rooks = board.byTypeBB[ROOK] & board.byColorBB[color];

    Bitboard occupied = board.byColorBB[WHITE] | board.byColorBB[BLACK];

    while (rooks) {
        int fromSquare = __builtin_ctzll(rooks);
        rooks &= rooks - 1;

        Bitboard attacks = magic::getRookAttacks(fromSquare, occupied) & ~allies;

        while (attacks) {
            int toSquare = __builtin_ctzll(attacks);
            moves.addMove(fromSquare, toSquare, NORMAL, 0);
            attacks &= attacks - 1;
        }
    }
}

void generateQueenMoves(const Board& board, MoveList& moves) {
    int color = board.turn;
    Bitboard allies = board.byColorBB[color];

    Bitboard queens = board.byTypeBB[QUEEN] & board.byColorBB[color];

    Bitboard occupied = board.byColorBB[WHITE] | board.byColorBB[BLACK];

    while (queens) {
        int fromSquare = __builtin_ctzll(queens);
        queens &= queens - 1;

        Bitboard attacks = magic::getQueenAttacks(fromSquare, occupied) & ~allies;

        while (attacks) {
            int toSquare = __builtin_ctzll(attacks);
            moves.addMove(fromSquare, toSquare, NORMAL, 0);
            attacks &= attacks - 1;
        }
    }
}

void generateKingMoves(const Board& board, MoveList& moves) {
    int color = board.turn;
    Bitboard allies = board.byColorBB[color];
    Bitboard occupied = board.byColorBB[WHITE] | board.byColorBB[BLACK];

    Bitboard king = board.byTypeBB[KING] & board.byColorBB[color];
    int fromSquare = __builtin_ctzll(king);

    // normal move
    Bitboard targets = magic::getKingAttacks(fromSquare) & ~allies;

    while (targets) {
        int toSquare = __builtin_ctzll(targets);
        moves.addMove(fromSquare, toSquare, NORMAL, 0);
        targets &= targets -1;
    }

    // castling
    if (color == WHITE && board.kingCastle[WHITE]) {
        if (!(occupied & ((1ULL << 5) | (1ULL << 6)))) {
            if (!board.isSquareAttacked(4, color ^ 1) 
            && !board.isSquareAttacked(5, color ^ 1)) {
                moves.addMove(4, 6, CASTLING, 0);
            }
        }
    }
    if (color == WHITE && board.queenCastle[WHITE]) {
        if (!(occupied & ((1ULL << 1) | (1ULL << 2) | (1ULL << 3)))) {
            if (!board.isSquareAttacked(4, color ^ 1) 
                && !board.isSquareAttacked(3, color ^ 1)) {
                moves.addMove(4, 2, CASTLING, 0);
            }
        }
    }

    if (color == BLACK && board.kingCastle[BLACK]) {
        if (!(occupied & ((1ULL << 61) | (1ULL << 62)))) {
            if (!board.isSquareAttacked(60, color ^ 1) 
                &&!board.isSquareAttacked(61, color ^ 1)) {
                moves.addMove(60, 62, CASTLING, 0);
            }
        }
    }
    if (color == BLACK && board.queenCastle[BLACK]) {
        if (!(occupied & ((1ULL << 57) | (1ULL << 58) | (1ULL << 59)))) {
            if (!board.isSquareAttacked(60, color ^ 1) 
                && !board.isSquareAttacked(59, color ^ 1)) {
                moves.addMove(60, 58, CASTLING, 0);
            }
        }
    }   
}

MoveList generateTacticalMoves(const Board& board) {
    MoveList moves;

    generateTacticalPawnMoves(board, moves);
    generateTacticalKnightMoves(board, moves);
    generateTacticalBishopMoves(board, moves);
    generateTacticalRookMoves(board, moves);
    generateTacticalQueenMoves(board, moves);
    generateTacticalKingMoves(board, moves);

    return moves;
}
void generateTacticalPawnMoves(const Board& board, MoveList& moves) {
    int color = board.turn;
    Bitboard empty = ~(board.byColorBB[WHITE] | board.byColorBB[BLACK]);
    Bitboard enemies = board.byColorBB[color ^ 1];

    Bitboard pawns = board.byTypeBB[PAWN] & board.byColorBB[color];

    // captures
    if (color == WHITE) {
        Bitboard targets = ((pawns & NOT_FILE8) << 9) & enemies & NOT_RANK8;
        while (targets) {
            int toSquare = __builtin_ctzll(targets);
            moves.addMove(toSquare - 9, toSquare, NORMAL);
            targets &= targets - 1;
        }
        targets = ((pawns & NOT_FILE1) << 7) & enemies & NOT_RANK8;
        while (targets) {
            int toSquare = __builtin_ctzll(targets);
            moves.addMove(toSquare - 7, toSquare, NORMAL);
            targets &= targets - 1;
        }
    }
    if (color == BLACK) {
        Bitboard targets = ((pawns & NOT_FILE8) >> 7) & enemies & NOT_RANK1;
        while (targets) {
            int toSquare = __builtin_ctzll(targets);
            moves.addMove(toSquare + 7, toSquare, NORMAL);
            targets &= targets - 1;
        }
        targets = ((pawns & NOT_FILE1) >> 9) & enemies & NOT_RANK1;
        while (targets) {
            int toSquare = __builtin_ctzll(targets);
            moves.addMove(toSquare + 9, toSquare, NORMAL);
            targets &= targets - 1;
        }
    }

    // en passant
    if (board.epSquare != NO_SQUARE) {
        Bitboard ep = 1ULL << board.epSquare;
        Bitboard attackers = 0;
        if (color == WHITE)
            attackers = (((ep & NOT_FILE1) >> 9) | ((ep & NOT_FILE8) >> 7)) & pawns;
        if (color == BLACK)
            attackers = (((ep & NOT_FILE8) << 9) | ((ep & NOT_FILE1) << 7)) & pawns;
        while (attackers) {
            int fromSquare = __builtin_ctzll(attackers);
            moves.addMove(fromSquare, board.epSquare, EN_PASSANT, 0);
            attackers &= attackers - 1;
        }
    }

    // no caputre promotions
    if (color == WHITE) {
        Bitboard targets = (pawns << 8) & empty & RANK8;
        while (targets) {
            int toSquare = __builtin_ctzll(targets);
            moves.addMove(toSquare - 8, toSquare, PROMOTION, QUEEN);
            moves.addMove(toSquare - 8, toSquare, PROMOTION, ROOK);
            moves.addMove(toSquare - 8, toSquare, PROMOTION, BISHOP);
            moves.addMove(toSquare - 8, toSquare, PROMOTION, KNIGHT);
            targets &= targets - 1;
        }
    }
    if (color == BLACK) {
        Bitboard targets = (pawns >> 8) & empty & RANK1;
        while (targets) {
            int toSquare = __builtin_ctzll(targets);
            moves.addMove(toSquare + 8, toSquare, PROMOTION, QUEEN);
            moves.addMove(toSquare + 8, toSquare, PROMOTION, ROOK);
            moves.addMove(toSquare + 8, toSquare, PROMOTION, BISHOP);
            moves.addMove(toSquare + 8, toSquare, PROMOTION, KNIGHT);
            targets &= targets - 1;
        }
    }

    // capture promotions
    if (color == WHITE) {
        Bitboard targets = ((pawns & NOT_FILE8) << 9) & enemies & RANK8;
        while (targets) {
            int toSquare = __builtin_ctzll(targets);
            moves.addMove(toSquare - 9, toSquare, PROMOTION, QUEEN);
            moves.addMove(toSquare - 9, toSquare, PROMOTION, ROOK);
            moves.addMove(toSquare - 9, toSquare, PROMOTION, BISHOP);
            moves.addMove(toSquare - 9, toSquare, PROMOTION, KNIGHT);
            targets &= targets - 1;
        }
        targets = ((pawns & NOT_FILE1) << 7) & enemies & RANK8;
        while (targets) {
            int toSquare = __builtin_ctzll(targets);
            moves.addMove(toSquare - 7, toSquare, PROMOTION, QUEEN);
            moves.addMove(toSquare - 7, toSquare, PROMOTION, ROOK);
            moves.addMove(toSquare - 7, toSquare, PROMOTION, BISHOP);
            moves.addMove(toSquare - 7, toSquare, PROMOTION, KNIGHT);
            targets &= targets - 1;
        }
    }
    if (color == BLACK) {
        Bitboard targets = ((pawns & NOT_FILE8) >> 7) & enemies & RANK1;
        while (targets) {
            int toSquare = __builtin_ctzll(targets);
            moves.addMove(toSquare + 7, toSquare, PROMOTION, QUEEN);
            moves.addMove(toSquare + 7, toSquare, PROMOTION, ROOK);
            moves.addMove(toSquare + 7, toSquare, PROMOTION, BISHOP);
            moves.addMove(toSquare + 7, toSquare, PROMOTION, KNIGHT);
            targets &= targets - 1;
        }
        targets = ((pawns & NOT_FILE1) >> 9) & enemies & RANK1;
        while (targets) {
            int toSquare = __builtin_ctzll(targets);
            moves.addMove(toSquare + 9, toSquare, PROMOTION, QUEEN);
            moves.addMove(toSquare + 9, toSquare, PROMOTION, ROOK);
            moves.addMove(toSquare + 9, toSquare, PROMOTION, BISHOP);
            moves.addMove(toSquare + 9, toSquare, PROMOTION, KNIGHT);
            targets &= targets - 1;
        }
    }
}

void generateTacticalKnightMoves(const Board& board, MoveList& moves) {
    int color = board.turn;
    Bitboard enemies = board.byColorBB[color ^ 1];
    Bitboard knights = board.byTypeBB[KNIGHT] & board.byColorBB[color];

    while (knights) {
        int fromSquare = __builtin_ctzll(knights);
        knights &= knights - 1;
        Bitboard targets = magic::getKnightAttacks(fromSquare) & enemies;
        while (targets) {
            int toSquare = __builtin_ctzll(targets);
            moves.addMove(fromSquare, toSquare, NORMAL, 0);
            targets &= targets - 1;
        }
    }
}

void generateTacticalBishopMoves(const Board& board, MoveList& moves) {
    int color = board.turn;
    Bitboard enemies = board.byColorBB[color ^ 1];
    Bitboard bishops = board.byTypeBB[BISHOP] & board.byColorBB[color];

    Bitboard occupied = board.byColorBB[WHITE] | board.byColorBB[BLACK];

    while (bishops) {
        int fromSquare = __builtin_ctzll(bishops);
        bishops &= bishops - 1;
        Bitboard attacks = magic::getBishopAttacks(fromSquare, occupied) & enemies;
        while (attacks) {
            int toSquare = __builtin_ctzll(attacks);
            moves.addMove(fromSquare, toSquare, NORMAL, 0);
            attacks &= attacks - 1;
        }
    }
}

void generateTacticalRookMoves(const Board& board, MoveList& moves) {
    int color = board.turn;
    Bitboard enemies = board.byColorBB[color ^ 1];
    Bitboard rooks = board.byTypeBB[ROOK] & board.byColorBB[color];

    Bitboard occupied = board.byColorBB[WHITE] | board.byColorBB[BLACK];

    while (rooks) {
        int fromSquare = __builtin_ctzll(rooks);
        rooks &= rooks - 1;
        Bitboard attacks = magic::getRookAttacks(fromSquare, occupied) & enemies;
        while (attacks) {
            int toSquare = __builtin_ctzll(attacks);
            moves.addMove(fromSquare, toSquare, NORMAL, 0);
            attacks &= attacks - 1;
        }
    }
}

void generateTacticalQueenMoves(const Board& board, MoveList& moves) {
    int color = board.turn;
    Bitboard enemies = board.byColorBB[color ^ 1];
    Bitboard queens = board.byTypeBB[QUEEN] & board.byColorBB[color];

    Bitboard occupied = board.byColorBB[WHITE] | board.byColorBB[BLACK];

    while (queens) {
        int fromSquare = __builtin_ctzll(queens);
        queens &= queens - 1;
        Bitboard attacks = magic::getQueenAttacks(fromSquare, occupied) & enemies;
        while (attacks) {
            int toSquare = __builtin_ctzll(attacks);
            moves.addMove(fromSquare, toSquare, NORMAL, 0);
            attacks &= attacks - 1;
        }
    }
}

void generateTacticalKingMoves(const Board& board, MoveList& moves) {
    int color = board.turn;
    Bitboard enemies = board.byColorBB[color ^ 1];
    Bitboard king = board.byTypeBB[KING] & board.byColorBB[color];
    int fromSquare = __builtin_ctzll(king);

    Bitboard targets = magic::getKingAttacks(fromSquare) & enemies;
    while (targets) {
        int toSquare = __builtin_ctzll(targets);
        moves.addMove(fromSquare, toSquare, NORMAL, 0);
        targets &= targets - 1;
    }
}