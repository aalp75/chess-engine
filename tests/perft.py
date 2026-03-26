import sys
import pytest

sys.path.insert(0, 'src')

from board import Board
import moves as _moves

def perft(board, depth):
    if depth == 0:
        return 1
    moves = _moves.generate_moves(board)
    count = 0
    side = board.turn

    for move in moves:
        saved = board.save_state()
        _moves.make_move(board, move)
        board.turn ^= 1
        if not board.is_in_check(side):
            count += perft(board, depth - 1)
        board.restore_state(saved)

    return count

START = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1"
KIWIPETE = "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq -"
POS3 = "8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - - 0 1"
POS4 = "r3k2r/Pppp1ppp/1b3nbN/nP6/BBP1P3/q4N2/Pp1P2PP/R2Q1RK1 w kq - 0 1"
POS5 = "rnbq1k1r/pp1Pbppp/2p5/8/2B5/8/PPP1NnPP/RNBQK2R w KQ - 1 8  "
POS6 = "r4rk1/1pp1qppp/p1np1n2/2b1p1B1/2B1P1b1/P1NP1N2/1PP1QPPP/R4RK1 w - - 0 10 "

@pytest.mark.parametrize("fen,depth,expected", [
    pytest.param(START,    1,      20, id="start-d1"),
    pytest.param(START,    2,     400, id="start-d2"),
    pytest.param(START,    3,    8902, id="start-d3"),
    pytest.param(START,    4,  197281, id="start-d4", marks=pytest.mark.slow),

    pytest.param(KIWIPETE, 1,      48, id="kiwipete-d1"),
    pytest.param(KIWIPETE, 2,    2039, id="kiwipete-d2"),
    pytest.param(KIWIPETE, 3,   97862, id="kiwipete-d3"),
    pytest.param(KIWIPETE, 3, 4085603, id="kiwipete-d4", marks=pytest.mark.slow),

    pytest.param(POS3,     1,      14, id="pos3-d1"),
    pytest.param(POS3,     2,     191, id="pos3-d2"),
    pytest.param(POS3,     3,    2812, id="pos3-d3"),
    pytest.param(POS3,     4,   43238, id="pos3-d4", marks=pytest.mark.slow),

    pytest.param(POS4,     1,       6, id="pos4-d1"),
    pytest.param(POS4,     2,     264, id="pos4-d2"),
    pytest.param(POS4,     3,    9467, id="pos4-d3"),
    pytest.param(POS4,     4,  422333, id="pos4-d4"),

    pytest.param(POS5,     1,      44, id="pos5-d1"),
    pytest.param(POS5,     2,    1486, id="pos5-d2"),
    pytest.param(POS5,     3,   62379, id="pos5-d3"),
    pytest.param(POS5,     4, 2103487, id="pos5-d4"),

    pytest.param(POS6,     1,      46, id="pos6-d1"),
    pytest.param(POS6,     2,    2079, id="pos6-d2"),
    pytest.param(POS6,     3,   89890, id="pos6-d3"),
    pytest.param(POS6,     4, 3894594, id="pos6-d4"),
])

def test_perft(fen, depth, expected):
    board = Board(fen=fen)
    assert perft(board, depth) == expected

if __name__ == "__main__":
    #pytest.main([__file__, "-v",])
    pytest.main([__file__, "-v", "-m", "not slow"])