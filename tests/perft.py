import copy

import sys
sys.path.insert(0, 'src')

from board import Board
import moves as _moves

def perft(board, depth):
    if depth == 0:
        return 1
    moves = _moves.generate_moves(board)
    count = 0

    for move in moves:
        board_copy = copy.deepcopy(board)
        _moves.make_move(board_copy, move)
        board_copy.turn ^= 1
        if board_copy.is_in_check(board.turn):
            continue
        count += perft(board_copy, depth - 1)

    return count


board = Board()

def divide(board, depth):
    moves = _moves.generate_moves(board)
    total = 0
    for move in moves:
        board_copy = copy.deepcopy(board)
        _moves.make_move(board_copy, move)
        board_copy.turn ^= 1
        if board_copy.is_in_check(board.turn):
            continue
        count = perft(board_copy, depth - 1)
        total += count
        files = 'abcdefgh'
        from_sq = move[0]
        to_sq   = move[1]
        from_name = files[from_sq % 8] + str(from_sq // 8 + 1)
        to_name   = files[to_sq % 8]   + str(to_sq // 8 + 1)
        print(f"{from_name}{to_name}: {count}")
    print(f"Total: {total}")

EXPECTED = [0, 20, 400, 8902, 197281, 4865609]

#res = divide(board, 4)

for depth in range(1, 5):
    count = perft(board, depth)
    print(f"depth {depth}: {count:,} (expected {EXPECTED[depth]:,})")

fen = "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq -"
board = Board(fen=fen)


EXPECTED = [0, 48, 2039, 97862, 4085603]

for depth in range(1, 4):
    count = perft(board, depth)
    print(f"depth {depth}: {count:,} (expected {EXPECTED[depth]:,})")

fen = "8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - - 0 1 "
board = Board(fen=fen)


EXPECTED = [0, 14, 191, 2812, 43238]

for depth in range(1, 5):
    count = perft(board, depth)
    print(f"depth {depth}: {count:,} (expected {EXPECTED[depth]:,})")


