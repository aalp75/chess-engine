from constants import  *
from board import Board

PIECE_VALUES = [0] * N_PIECES

PIECE_VALUES[PAWN] = 100
PIECE_VALUES[KNIGHT] = 300
PIECE_VALUES[BISHOP] = 300
PIECE_VALUES[ROOK] = 500
PIECE_VALUES[QUEEN] = 900
PIECE_VALUES[KING] = 0

def evaluate(board):
    score = 0
    for piece in PIECES:
        score += board.bitboards[piece][WHITE].bit_count() * PIECE_VALUES[piece]
        score -= board.bitboards[piece][BLACK].bit_count() * PIECE_VALUES[piece]
    # return from the perspective of the side to move
    return score if board.turn == WHITE else -score
    #return 0