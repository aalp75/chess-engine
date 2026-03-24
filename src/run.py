from board import Board
import moves
from constants import *

def play():
    board = Board()
    while True:
        print(board)
        # long algebraic notation for now
        move = input(f"Enter move ({COLORS[board.turn]} to play): ")
        moves.make_move_str(board, move)
        board.turn ^= 1

play()
