#include<vector>

#include "moves.h"
#include "constants.h"
#include "board.h"
#include "moveList.h"
#include "magic.h"

/*

TODO: clean a bit the Zobrist incremental update

type:

NORMAL     = 0
EN_PASSANT = 1
CASTLING   = 2
PROMOTION  = 3

Note: __builtin_ctzll count trailing zeros in the binary representation
__builtin_ctzll(12) = 2 because 12 = 1100

*/

void doMove(Board& board, Move move, StateInfo* states, int ply) {
    int fromSquare = moveFrom(move);
    int toSquare = moveTo(move);
    int type = moveType(move);
    int promoPiece = movePromo(move);

    int color = board.turn;
    int enemyColor = color ^ 1;

    int piece = board.pieceBoard[fromSquare];
    int captured = (type == EN_PASSANT) ? PAWN : board.pieceBoard[toSquare];

    StateInfo& newState = states[ply];

    newState.fromSquare = fromSquare;
    newState.toSquare = toSquare;

    newState.movedPiece = piece;
    newState.capturedPiece = captured;
    newState.kingCastle[0] = board.kingCastle[0]; 
    newState.kingCastle[1] = board.kingCastle[1];
    newState.queenCastle[0] = board.queenCastle[0];
    newState.queenCastle[1] = board.queenCastle[1];
    newState.enPassant = board.enPassant;
    newState.type = type;
    newState.enPassantSquare = board.enPassantSquare;
    newState.promoPiece = promoPiece;
    newState.key = board.key;

    int oldCastle = (board.kingCastle[WHITE]  ? 1 : 0) | (board.queenCastle[WHITE] ? 2 : 0)
                  | (board.kingCastle[BLACK]  ? 4 : 0) | (board.queenCastle[BLACK] ? 8 : 0);

    zobrist::updateKeyCastle(board.key, oldCastle);

    if (board.enPassant) {
        zobrist::updateKeyEnPassant(board.key, board.enPassantSquare);
    }

    board.enPassant = false;

    zobrist::updateKeyPiece(board.key, piece, color, fromSquare);
    int landingPiece = (type == PROMOTION) ? promoPiece : piece;
    zobrist::updateKeyPiece(board.key, landingPiece, color, toSquare);

    if (captured != NO_PIECE && type != EN_PASSANT)
        zobrist::updateKeyPiece(board.key, captured, enemyColor, toSquare);

    // set en passant square for double pawn push
    if (piece == PAWN && std::abs(toSquare - fromSquare) == 16) {
        board.enPassant = true;
        board.enPassantSquare = (fromSquare + toSquare) / 2;
        zobrist::updateKeyEnPassant(board.key, board.enPassantSquare);
    }

    board.bitboards[piece][color] &= ~(1ULL << fromSquare);
    board.bitboards[piece][color] |= (1ULL << toSquare);

    board.wbPieces[color] &= ~(1ULL << fromSquare);
    board.wbPieces[color] |= (1ULL << toSquare);

    board.pieceBoard[fromSquare] = NO_PIECE;
    board.pieceBoard[toSquare] = piece;

    if (captured != NO_PIECE) {
        board.bitboards[captured][enemyColor] &= ~(1ULL << toSquare);
        board.wbPieces[enemyColor] &= ~(1ULL << toSquare);
    }

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
        board.bitboards[PAWN][enemyColor] &= ~ (1ULL << capturedSquare);

        board.wbPieces[enemyColor] &= ~(1ULL << capturedSquare);
        board.pieceBoard[capturedSquare] = NO_PIECE;

        newState.capturedSquare = capturedSquare;
        zobrist::updateKeyPiece(board.key, PAWN, enemyColor, capturedSquare);
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
        board.bitboards[PAWN][color] &= ~(1ULL << toSquare);
        board.bitboards[promoPiece][color] |= (1ULL << toSquare);
        board.pieceBoard[toSquare] = promoPiece;
        newState.promoPiece = promoPiece;
    }

    if (type == CASTLING) {
        
        if (toSquare == 6) { // white O-O
            board.bitboards[ROOK][WHITE] &= ~(1ULL << 7);
            board.bitboards[ROOK][WHITE] |=  (1ULL << 5);
            board.wbPieces[WHITE] &= ~(1ULL << 7);
            board.wbPieces[WHITE] |=  (1ULL << 5);
            board.pieceBoard[7] = NO_PIECE;
            board.pieceBoard[5] = ROOK;
            zobrist::updateKeyPiece(board.key, ROOK, WHITE, 7);
            zobrist::updateKeyPiece(board.key, ROOK, WHITE, 5);
        }
        else if (toSquare == 2) { // white O-O-O
            board.bitboards[ROOK][WHITE] &= ~(1ULL << 0);
            board.bitboards[ROOK][WHITE] |=  (1ULL << 3);
            board.wbPieces[WHITE] &= ~(1ULL << 0);
            board.wbPieces[WHITE] |=  (1ULL << 3);
            board.pieceBoard[0] = NO_PIECE;
            board.pieceBoard[3] = ROOK;
            zobrist::updateKeyPiece(board.key, ROOK, WHITE, 0);
            zobrist::updateKeyPiece(board.key, ROOK, WHITE, 3);
        }
        else if (toSquare == 62) { // black O-O
            board.bitboards[ROOK][BLACK] &= ~(1ULL << 63);
            board.bitboards[ROOK][BLACK] |=  (1ULL << 61);
            board.wbPieces[BLACK] &= ~(1ULL << 63);
            board.wbPieces[BLACK] |=  (1ULL << 61);
            board.pieceBoard[63] = NO_PIECE;
            board.pieceBoard[61] = ROOK;
            zobrist::updateKeyPiece(board.key, ROOK, BLACK, 63);
            zobrist::updateKeyPiece(board.key, ROOK, BLACK, 61);
        }
        else if (toSquare == 58) { // black O-O-O
            board.bitboards[ROOK][BLACK] &= ~(1ULL << 56);
            board.bitboards[ROOK][BLACK] |=  (1ULL << 59);
            board.wbPieces[BLACK] &= ~(1ULL << 56);
            board.wbPieces[BLACK] |=  (1ULL << 59);
            board.pieceBoard[56] = NO_PIECE;
            board.pieceBoard[59] = ROOK;
            zobrist::updateKeyPiece(board.key, ROOK, BLACK, 56);
            zobrist::updateKeyPiece(board.key, ROOK, BLACK, 59);
        }
    }

    board.occupied = board.wbPieces[WHITE] | board.wbPieces[BLACK];

    zobrist::updateKeyside(board.key);
    board.turn ^= 1;
}

void undoMove(Board& board, StateInfo* states, int ply) {

    StateInfo& state = states[ply];

    int fromSquare = state.fromSquare;
    int toSquare = state.toSquare;
    int piece = state.movedPiece;
    int captured = state.capturedPiece;
    int color = board.turn ^ 1;
    int enemyColor = color ^ 1;
    int promoPiece = state.promoPiece;

    if (promoPiece != NO_PIECE) {
        board.bitboards[promoPiece][color] &= ~(1ULL << toSquare);
    }
    
    // move piece back
    board.bitboards[piece][color] &= ~(1ULL << toSquare);
    board.bitboards[piece][color] |=  (1ULL << fromSquare);
    board.wbPieces[color] &= ~(1ULL << toSquare);
    board.wbPieces[color] |=  (1ULL << fromSquare);
    board.pieceBoard[toSquare]   = NO_PIECE;
    board.pieceBoard[fromSquare] = piece;

    // restored captured piece (not for en passant, handled separately below)
    if (captured != NO_PIECE && state.type != EN_PASSANT) {
        board.bitboards[captured][enemyColor] |=  (1ULL << toSquare);
        board.wbPieces[enemyColor]            |=  (1ULL << toSquare);
        board.pieceBoard[toSquare] = captured;
    }

    // undo en passant
    if (state.type == EN_PASSANT) {
        int capturedSquare = (color == WHITE) ? toSquare - 8 : toSquare + 8;
        board.bitboards[PAWN][enemyColor] |= (1ULL << capturedSquare);
        board.wbPieces[enemyColor]        |= (1ULL << capturedSquare);
        board.pieceBoard[capturedSquare]   = PAWN;
    }

    // undo promotion (restore pawn, clear promoted piece)
    if (state.type == PROMOTION) {
        board.bitboards[state.promoPiece][color] &= ~(1ULL << toSquare);
        board.bitboards[piece][color]            &= ~(1ULL << fromSquare);
        board.bitboards[PAWN][color]             |=  (1ULL << fromSquare);
        board.pieceBoard[fromSquare]              = PAWN;
    }

    // undo castling (move rook back)
    if (state.type == CASTLING) {
        if (toSquare == 6) {
            board.bitboards[ROOK][WHITE] &= ~(1ULL << 5);
            board.bitboards[ROOK][WHITE] |=  (1ULL << 7);
            board.wbPieces[WHITE] &= ~(1ULL << 5);
            board.wbPieces[WHITE] |=  (1ULL << 7);
            board.pieceBoard[5] = NO_PIECE;
            board.pieceBoard[7] = ROOK;
        } else if (toSquare == 2) {
            board.bitboards[ROOK][WHITE] &= ~(1ULL << 3);
            board.bitboards[ROOK][WHITE] |=  (1ULL << 0);
            board.wbPieces[WHITE] &= ~(1ULL << 3);
            board.wbPieces[WHITE] |=  (1ULL << 0);
            board.pieceBoard[3] = NO_PIECE;
            board.pieceBoard[0] = ROOK;
        } else if (toSquare == 62) {
            board.bitboards[ROOK][BLACK] &= ~(1ULL << 61);
            board.bitboards[ROOK][BLACK] |=  (1ULL << 63);
            board.wbPieces[BLACK] &= ~(1ULL << 61);
            board.wbPieces[BLACK] |=  (1ULL << 63);
            board.pieceBoard[61] = NO_PIECE;
            board.pieceBoard[63] = ROOK;
        } else if (toSquare == 58) {
            board.bitboards[ROOK][BLACK] &= ~(1ULL << 59);
            board.bitboards[ROOK][BLACK] |=  (1ULL << 56);
            board.wbPieces[BLACK] &= ~(1ULL << 59);
            board.wbPieces[BLACK] |=  (1ULL << 56);
            board.pieceBoard[59] = NO_PIECE;
            board.pieceBoard[56] = ROOK;
        }
    }

    // restore board state
    board.kingCastle[0]    = state.kingCastle[0];
    board.kingCastle[1]    = state.kingCastle[1];
    board.queenCastle[0]   = state.queenCastle[0];
    board.queenCastle[1]   = state.queenCastle[1];
    board.enPassant        = state.enPassant;
    board.enPassantSquare  = state.enPassantSquare;
    board.occupied         = board.wbPieces[WHITE] | board.wbPieces[BLACK];
    board.turn             ^= 1;
    board.key              =state.key;

}

void doNullMove(Board& board, StateInfo* states, int ply) {
    StateInfo& newState = states[ply];
    newState.enPassant       = board.enPassant;
    newState.enPassantSquare = board.enPassantSquare;
    newState.key             = board.key;

    if (board.enPassant) {
        zobrist::updateKeyEnPassant(board.key, board.enPassantSquare);
        board.enPassant = false;
    }

    zobrist::updateKeyside(board.key);
    board.turn ^= 1;
}

void undoNullMove(Board& board, StateInfo* states, int ply) {
    StateInfo& state = states[ply];
    board.key             = state.key;
    board.enPassant       = state.enPassant;
    board.enPassantSquare = state.enPassantSquare;
    board.turn            ^= 1;
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
    Bitboard empty = ~board.occupied;
    Bitboard enemies = board.wbPieces[color ^ 1];
    
    // single push
    if (color == WHITE) {
        Bitboard targets = (board.bitboards[PAWN][color] << 8) & empty & NOT_RANK8;

        while (targets) {
            int toSquare = __builtin_ctzll(targets);
            int fromSquare = toSquare - 8;
            moves.addMove(fromSquare, toSquare, NORMAL);
            targets &= targets - 1;
        }
    }

    if (color == BLACK) {
        Bitboard targets = (board.bitboards[PAWN][color] >> 8) & empty & NOT_RANK1;

        while (targets) {
            int toSquare = __builtin_ctzll(targets);
            int fromSquare = toSquare + 8;
            moves.addMove(fromSquare, toSquare, NORMAL);
            targets &= targets - 1;
        }
    }

    // double push
    if (color == WHITE) {
        Bitboard targets = ((board.bitboards[PAWN][color] & RANK2) << 16) & empty & (empty << 8);

        while (targets) {
            int toSquare = __builtin_ctzll(targets);
            int fromSquare = toSquare - 16;
            moves.addMove(fromSquare, toSquare, NORMAL);
            targets &= targets - 1;
        }
    }

    if (color == BLACK) {
        Bitboard targets = ((board.bitboards[PAWN][color] & RANK7) >> 16) & empty & (empty >> 8);

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
        Bitboard targets = ((board.bitboards[PAWN][color] & NOT_FILE8) << 9) & enemies & NOT_RANK8;

        while (targets) {
            int toSquare = __builtin_ctzll(targets);
            int fromSquare = toSquare - 9;
            moves.addMove(fromSquare, toSquare, NORMAL);
            targets &= targets - 1;
        }

        // captures left (exclude rank 8, it's handled by capture promotions)
        targets = ((board.bitboards[PAWN][color] & NOT_FILE1) << 7) & enemies & NOT_RANK8;

        while (targets) {
            int toSquare = __builtin_ctzll(targets);
            int fromSquare = toSquare - 7;
            moves.addMove(fromSquare, toSquare, NORMAL);
            targets &= targets - 1;
        }
    }

    if (color == BLACK) {
        // captures right (exclude rank 1, it's handled by capture promotions)
        Bitboard targets = ((board.bitboards[PAWN][color] & NOT_FILE8) >> 7) & enemies & NOT_RANK1;

        while (targets) {
            int toSquare = __builtin_ctzll(targets);
            int fromSquare = toSquare + 7;
            moves.addMove(fromSquare, toSquare, NORMAL);
            targets &= targets - 1;
        }

        // captures left (exclude rank 1, it's handled by capture promotions)
        targets = ((board.bitboards[PAWN][color] & NOT_FILE1) >> 9) & enemies & NOT_RANK1;

        while (targets) {
            int toSquare = __builtin_ctzll(targets);
            int fromSquare = toSquare + 9;
            moves.addMove(fromSquare, toSquare, NORMAL);
            targets &= targets - 1;
        }
    }

    // en passant
    if (board.enPassant) {
        Bitboard ep = 1ULL << board.enPassantSquare;
        Bitboard attackers = 0;
        if (color == WHITE) {
            attackers = (((ep & NOT_FILE1) >> 9) | ((ep & NOT_FILE8) >> 7)) & board.bitboards[PAWN][color];
        }
        if (color == BLACK) {
            attackers = (((ep & NOT_FILE8) << 9) | ((ep & NOT_FILE1) << 7)) & board.bitboards[PAWN][color];
        }

        while (attackers) {
            int fromSquare = __builtin_ctzll(attackers);
            moves.addMove(fromSquare, board.enPassantSquare, EN_PASSANT, 0);
            attackers &= attackers - 1;
        }
    }

    // promotions
    if (color == WHITE) {
        Bitboard targets = (board.bitboards[PAWN][color] << 8) & empty & RANK8;

        while (targets) {
            int toSquare = __builtin_ctzll(targets);
            int fromSquare = toSquare - 8;
            moves.addMove(fromSquare, toSquare, PROMOTION, QUEEN);
            moves.addMove(fromSquare, toSquare, PROMOTION, ROOK);
            moves.addMove(fromSquare, toSquare, PROMOTION, BISHOP);
            moves.addMove(fromSquare, toSquare, PROMOTION, KNIGHT);
            targets &= targets - 1;
        }
    }

    if (color == BLACK) {
        Bitboard targets = (board.bitboards[PAWN][color] >> 8) & empty & RANK1;

        while (targets) {
            int toSquare = __builtin_ctzll(targets);
            int fromSquare = toSquare + 8;
            moves.addMove(fromSquare, toSquare, PROMOTION, QUEEN);
            moves.addMove(fromSquare, toSquare, PROMOTION, ROOK);
            moves.addMove(fromSquare, toSquare, PROMOTION, BISHOP);
            moves.addMove(fromSquare, toSquare, PROMOTION, KNIGHT);
            targets &= targets - 1;
        }
    }

    // capture promotions
    if (color == WHITE) {
        // capture right + promote
        Bitboard targets = ((board.bitboards[PAWN][color] & NOT_FILE8) << 9) & enemies & RANK8;

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
        targets = ((board.bitboards[PAWN][color] & NOT_FILE1) << 7) & enemies & RANK8;

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
        Bitboard targets = ((board.bitboards[PAWN][color] & NOT_FILE8) >> 7) & enemies & RANK1;

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
        targets = ((board.bitboards[PAWN][color] & NOT_FILE1) >> 9) & enemies & RANK1;

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
    Bitboard empty = ~board.occupied;
    Bitboard allies = board.wbPieces[color];
    Bitboard enemies = board.wbPieces[color ^ 1];

    Bitboard knights = board.bitboards[KNIGHT][color];

    while (knights) {
        int fromSquare = __builtin_ctzll(knights);
        Bitboard squareMask = 1ULL << fromSquare;
        knights &= knights - 1;

        Bitboard targets = 0;

        targets |= (squareMask & NOT_FILE8) << 17;
        targets |= (squareMask & NOT_FILE8 & NOT_FILE7) << 10;
        targets |= (squareMask & NOT_FILE8 & NOT_FILE7) >> 6;
        targets |= (squareMask & NOT_FILE8) >> 15;
        targets |= (squareMask & NOT_FILE1) << 15;
        targets |= (squareMask & NOT_FILE1 & NOT_FILE2) << 6;
        targets |= (squareMask & NOT_FILE1 & NOT_FILE2) >> 10;
        targets |= (squareMask & NOT_FILE1) >> 17;

        targets &= ~allies;

        while (targets) {
            int toSquare = __builtin_ctzll(targets);
            moves.addMove(fromSquare, toSquare, NORMAL, 0);
            targets &= targets - 1;
        }
    }
}

void generateBishopMoves(const Board& board, MoveList& moves) {
    int color = board.turn;
    Bitboard empty = ~board.occupied;
    Bitboard allies = board.wbPieces[color];
    Bitboard enemies = board.wbPieces[color ^ 1];

    Bitboard bishops = board.bitboards[BISHOP][color];

    while (bishops) {
        int fromSquare = __builtin_ctzll(bishops);
        Bitboard squareMask = 1ULL << fromSquare;
        bishops &= bishops - 1;

        for (int d = 0; d < 4; d++) { // up-right, up-left, down-right, down-left
            int direction = BISHOP_DIRECTIONS[d];
            int square = fromSquare;
            while (true) {
                int square_previous = square;
                square += direction;
                if (square < 0 || square > 63) { // out of board
                    break;
                }
                if (abs((square % 8) - (square_previous % 8)) != 1) {
                    break;
                }
                Bitboard target = 1ULL << square;
                if (target & allies) {
                    break;
                }
                if (target & enemies) {
                    moves.addMove(fromSquare, square, NORMAL, 0);
                    break;
                }
                moves.addMove(fromSquare, square, NORMAL, 0);
            }
        }
    }
}

void generateRookMoves(const Board& board, MoveList& moves) {
    int color = board.turn;
    Bitboard empty = ~board.occupied;
    Bitboard allies = board.wbPieces[color];
    Bitboard enemies = board.wbPieces[color ^ 1];

    Bitboard rooks = board.bitboards[ROOK][color];

    while (rooks) {
        int fromSquare = __builtin_ctzll(rooks);
        Bitboard squareMask = 1ULL << fromSquare;
        rooks &= rooks - 1;

        for (int d = 0; d < 4; d++) { // up, down, right, left
            int direction = ROOK_DIRECTIONS[d];
            int square = fromSquare;
            while (true) {
                int square_previous = square;
                square += direction;
                if (square < 0 || square > 63) { // out of board
                    break;
                }
                if (abs(direction) == 1 && square / 8 != square_previous / 8) {
                    break;
                }

                Bitboard target = 1ULL << square;
                if (target & allies) {
                    break;
                }
                if (target & enemies) {
                    moves.addMove(fromSquare, square, NORMAL, 0);
                    break;
                }
                moves.addMove(fromSquare, square, NORMAL, 0);
            }
        }
    }
}

void generateQueenMoves(const Board& board, MoveList& moves) {
    int color = board.turn;
    Bitboard empty = ~board.occupied;
    Bitboard allies = board.wbPieces[color];
    Bitboard enemies = board.wbPieces[color ^ 1];

    Bitboard queens = board.bitboards[QUEEN][color];

    while (queens) {
        int fromSquare = __builtin_ctzll(queens);
        Bitboard squareMask = 1ULL << fromSquare;
        queens &= queens - 1;

        for (int d = 0; d < 8; d++) { // up, down, right, left then diagonals
            int direction = QUEEN_DIRECTIONS[d];
            int square = fromSquare;
            while (true) {
                int square_previous = square;
                square += direction;
                if (square < 0 || square > 63) { // out of board
                    break;
                }
                if (abs((square % 8) - (square_previous % 8)) > 1) {
                    break;
                }
                
                Bitboard target = 1ULL << square;
                if (target & allies) {
                    break;
                }
                if (target & enemies) {
                    moves.addMove(fromSquare, square, NORMAL, 0);
                    break;
                }
                moves.addMove(fromSquare, square, NORMAL, 0);
            }
        }
    }
}
void generateKingMoves(const Board& board, MoveList& moves) {
    int color = board.turn;
    Bitboard empty = ~board.occupied;
    Bitboard allies = board.wbPieces[color];
    Bitboard enemies = board.wbPieces[color ^ 1];
    Bitboard occupied = board.occupied;

    Bitboard king = board.bitboards[KING][color];

    // normal move
    Bitboard targets = 0;

    targets |= (king & NOT_FILE8) << 9; // up-right
    targets |= king << 8;               // up
    targets |= (king & NOT_FILE1) << 7; // up-left
    targets |= (king & NOT_FILE8) << 1; // right
    targets |= (king & NOT_FILE1) >> 1; // left
    targets |= (king & NOT_FILE8) >> 7; // down-right
    targets |= king >> 8;               // down
    targets |= (king & NOT_FILE1) >> 9; // down-left

    targets &= ~allies;

    int fromSquare = __builtin_ctzll(king);

    while (targets) {
        int toSquare = __builtin_ctzll(targets);
        moves.addMove(fromSquare, toSquare, NORMAL, 0);
        targets &= targets -1;
    }

    // castling

    if (color == WHITE && board.kingCastle[WHITE]) {
        if (!(occupied & ((1ULL << 5) | (1ULL << 6)))) {
            if (!board.isSquareAttacked(4, color ^ 1) &&
                !board.isSquareAttacked(5, color ^ 1) &&
                !board.isSquareAttacked(6, color ^ 1)) {
                moves.addMove(4, 6, CASTLING, 0);
            }
        }
    }
    if (color == WHITE && board.queenCastle[WHITE]) {
        if (!(occupied & ((1ULL << 1) | (1ULL << 2) | (1ULL << 3)))) {
            if (!board.isSquareAttacked(4, color ^ 1) &&
                !board.isSquareAttacked(3, color ^ 1) &&
                !board.isSquareAttacked(2, color ^ 1)) {
                moves.addMove(4, 2, CASTLING, 0);
            }
        }
    }

    if (color == BLACK && board.kingCastle[BLACK]) {
        if (!(occupied & ((1ULL << 61) | (1ULL << 62)))) {
            if (!board.isSquareAttacked(60, color ^ 1) &&
                !board.isSquareAttacked(61, color ^ 1) &&
                !board.isSquareAttacked(62, color ^ 1)) {
                moves.addMove(60, 62, CASTLING, 0);
            }
        }
    }
    if (color == BLACK && board.queenCastle[BLACK]) {
        if (!(occupied & ((1ULL << 57) | (1ULL << 58) | (1ULL << 59)))) {
            if (!board.isSquareAttacked(60, color ^ 1) &&
                !board.isSquareAttacked(59, color ^ 1) &&
                !board.isSquareAttacked(58, color ^ 1)) {
                moves.addMove(60, 58, CASTLING, 0);
            }
        }
    }   
}
