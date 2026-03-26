#!/home/antoine/anaconda3/bin/python
import sys
from pathlib import Path

SRC_DIR = Path(__file__).parent
ROOT_DIR = SRC_DIR.parent
sys.path.insert(0, str(SRC_DIR))

from board import Board
from moves import generate_moves, make_move
from search import best_move
from constants import *

PROMO_CHAR = {'q': QUEEN, 'r': ROOK, 'b': BISHOP, 'n': KNIGHT}
PROMO_CHAR_REV = {v: k for k, v in PROMO_CHAR.items()}

def sq_to_str(sq):
    return 'abcdefgh'[sq % 8] + str(sq // 8 + 1)

def move_to_uci(move):
    s = sq_to_str(move[0]) + sq_to_str(move[1])
    if len(move) == 4:
        s += PROMO_CHAR_REV[move[3]]
    return s

def uci_to_move(board, uci):
    for move in generate_moves(board):
        if move_to_uci(move) == uci:
            return move
    return None

def apply_moves(board, tokens):
    for uci in tokens:
        move = uci_to_move(board, uci)
        if move:
            make_move(board, move)
            board.next_turn()

def main():
    import datetime
    log_dir = ROOT_DIR / 'logs'
    log_dir.mkdir(exist_ok=True)
    log_path = log_dir / f'{datetime.datetime.now().strftime("%Y%m%d_%H%M%S")}.log'
    log = open(log_path, 'w', buffering=1)

    board = Board()

    while True:
        line = input().strip()
        log.write(f'< {line}\n')

        if line == 'uci':
            print('id name Kasparov')
            print('id author Me')
            print('option name Move Overhead type spin default 10 min 0 max 5000')
            print('option name Threads type spin default 1 min 1 max 1')
            print('uciok')

        elif line == 'isready':
            print('readyok')

        elif line.startswith('setoption'):
            pass  # options not implemented

        elif line == 'ucinewgame':
            board = Board()

        elif line.startswith('position'):
            tokens = line.split()
            if tokens[1] == 'startpos':
                board = Board()
                if 'moves' in tokens:
                    apply_moves(board, tokens[tokens.index('moves') + 1:])
            elif tokens[1] == 'fen':
                fen_end = tokens.index('moves') if 'moves' in tokens else len(tokens)
                fen = ' '.join(tokens[2:fen_end])
                board = Board(fen=fen)
                if 'moves' in tokens:
                    apply_moves(board, tokens[tokens.index('moves') + 1:])

        elif line.startswith('go'):
            try:
                tokens = line.split()
                depth = 4
                if 'depth' in tokens:
                    depth = int(tokens[tokens.index('depth') + 1])
                elif 'movetime' in tokens:
                    movetime = int(tokens[tokens.index('movetime') + 1])
                    depth = depth if movetime >= 3000 else depth - 1
                move = best_move(board, depth)
                if move:
                    reply = f'bestmove {move_to_uci(move)}'
                else:
                    reply = 'bestmove 0000'#
                log.write(f'> {reply} (depth {depth})\n')
                print(reply)
                
            except Exception:
                import traceback
                log.write(traceback.format_exc())
                print('bestmove 0000')

        elif line == 'quit':
            break

if __name__ == '__main__':
    main()