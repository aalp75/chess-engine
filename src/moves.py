from constants import *

def get_square(notation):
    file = ord(notation[0]) - ord('a')
    rank = int(notation[1]) - 1
    return rank * 8 + file

def make_move(board, move):

    start = move[:2]
    end = move[-2:]
    eat = 'x' in move

    square_start = get_square(start)
    square_end = get_square(end)

    # find the piece on the start position
    for color in range(N_COLORS):
        for piece in range(N_PIECES):
            if board.bitboards[piece][color] & (1 << square_start):
                board.bitboards[piece][color] &= ~(1 << square_start)
                board.bitboards[piece][color] |= 1 << square_end
                return

    raise ValueError("Invalid move")

def generate_moves(board):
    moves = []
    generate_pawn_moves(board, moves)
    generate_knight_moves(board, moves)
    generate_bishop_moves(board, moves)
    generate_rook_moves(board, moves)
    generate_queen_moves(board, moves)
    generate_king_moves(board, moves)

    return moves

def generate_pawn_moves(board, moves):
    color = board.turn
    pawns = board.bitboards[PAWN][color]

    empty = board.get_empty_squares()

    # single push
    if color == WHITE:
        targets = (pawns << 8) & empty
        # targets is all potential next position
        while targets:
            lowest_bit = targets & -targets
            square = lowest_bit.bit_length() - 1
            from_square = square - 8
            moves.append((from_square, square))
            targets &= targets - 1 # remove that square from targets

    if color == BLACK:
        targets = (pawns >> 8) & empty
        # targets is all potential next position
        while targets:
            lowest_bit = targets & -targets
            square = lowest_bit.bit_length() - 1
            from_square = square + 8
            moves.append((from_square, square))
            targets &= targets - 1 # remove that square from targets

    # double push
    if color == WHITE:
        targets = ((pawns & RANK2) << 16) & empty & (empty << 8)
        while targets:
            lowest_bit = targets & -targets
            square = lowest_bit.bit_length() - 1
            from_square = square - 16
            moves.append((from_square, square))
            targets &= targets - 1

    if color == BLACK:
        targets = ((pawns & RANK7) >> 16) & empty & (empty >> 8)
        while targets:
            lowest_bit = targets & -targets
            square = lowest_bit.bit_length() - 1
            from_square = square + 16
            moves.append((from_square, square))
            targets &= targets - 1

    # captures left / right
    enemies = board.get_occupied_squares(color ^ 1)

    if color == WHITE:
        # captures right
        targets = ((pawns & NOT_FILE8) << 9) & enemies
        while targets:
            lowest_bit = targets & -targets
            square = lowest_bit.bit_length() - 1
            from_square = square - 9
            moves.append((from_square, square))
            targets &= targets - 1

        # captures left
        targets = ((pawns & NOT_FILE1) << 7) & enemies
        while targets:
            lowest_bit  = targets & -targets
            square = lowest_bit.bit_length() - 1
            from_square = square - 7
            moves.append((from_square, square))
            targets &= targets - 1

    if color == BLACK:
        # captures left
        targets = ((pawns & NOT_FILE8) >> 9) & enemies
        while targets:
            lowest_bit = targets & -targets
            square = lowest_bit.bit_length() - 1
            from_square = square + 9
            moves.append((from_square, square))
            targets &= targets - 1

        # captures right
        targets = ((pawns & NOT_FILE1) >> 7) & enemies
        while targets:
            lowest_bit  = targets & -targets
            square = lowest_bit.bit_length() - 1
            from_square = square + 7
            moves.append((from_square, square))
            targets &= targets - 1

    # en passant
    if board.en_passant_square is not None:
        ep_bb = 1 << board.en_passant_square
        if color == WHITE:
            attackers = ((ep_bb & NOT_FILE8) >> 9 | (ep_bb & NOT_FILE1) >> 7) & pawns


    # promotions

def generate_knight_moves(board, moves):
    color = board.turn
    knights = board.bitboards[KNIGHT][color]
    allies = board.get_occupied_squares(color)
    enemies = board.get_occupied_squares(color ^ 1)

    while knights:
        lowest_bit = knights & -knights
        from_square = lowest_bit.bit_length() - 1
        knights &= knights - 1

        targets  = 0
        targets |= (lowest_bit & NOT_FILE8) << 17
        targets |= (lowest_bit & NOT_FILE8 & NOT_FILE7)  << 10
        targets |= (lowest_bit & NOT_FILE8 & NOT_FILE7)  >> 6
        targets |= (lowest_bit & NOT_FILE8) >> 15
        targets |= (lowest_bit & NOT_FILE1) << 15
        targets |= (lowest_bit & NOT_FILE1 & NOT_FILE2)  << 6
        targets |= (lowest_bit & NOT_FILE1 & NOT_FILE2)  >> 10
        targets |= (lowest_bit & NOT_FILE1) >> 17

        targets &= ~allies & FULL_BOARD

        while targets:
            lowest_bit = targets & -targets
            square = lowest_bit.bit_length() - 1
            if (1 << square) & enemies:
                moves.append((from_square, square, CAPTURE))
            else:
                moves.append((from_square, square, QUIET))
            targets &= targets - 1

def generate_bishop_moves(board, moves):
    color = board.turn
    bishops = board.bitboards[BISHOP][color]
    allies = board.get_occupied_squares(color)
    enemies = board.get_occupied_squares(color ^ 1)

    while bishops:
        lowest_bit = bishops & -bishops
        from_square = lowest_bit.bit_length() - 1
        bishops &= bishops - 1

        for direction in [9, 7, -7, -9]:  # up-right / up-left / down-right / down-left
            square = from_square
            while True:
                square_previous = square
                square += direction
                if square < 0 or square > 63:
                    break
                if abs(square % 8 - square_previous % 8) != 1:
                    break
                target = 1 << square
                if target & allies:
                    break
                if target & enemies:
                    moves.append((from_square, square, CAPTURE))
                    break
                moves.append((from_square, square, QUIET))

def generate_rook_moves(board, moves):
    color = board.turn
    rooks = board.bitboards[ROOK][color]
    allies = board.get_occupied_squares(color)
    enemies = board.get_occupied_squares(color ^ 1)

    while rooks:
        lowest_bit = rooks & -rooks
        from_square = lowest_bit.bit_length() - 1
        rooks &= rooks - 1

        for direction in [8, -8, 1, -1]: # up / down / right / left
            square = from_square
            while True:
                square_previous = square
                square += direction
                if square < 0 or square > 63:
                    break
                if abs(direction) == 1 and square // 8 != square_previous // 8:
                    break
                target = 1 << square
                if target & allies:
                    break
                if target & enemies:
                    moves.append((from_square, square, CAPTURE))
                    break
                moves.append((from_square, square, QUIET))

def generate_queen_moves(board, moves):
    color = board.turn
    queens = board.bitboards[QUEEN][color]
    allies = board.get_occupied_squares(color)
    enemies = board.get_occupied_squares(color ^ 1)

    while queens:
        lowest_bit = queens & -queens
        from_square = lowest_bit.bit_length() - 1
        queens &= queens - 1

        for direction in [8, -8, 1, -1, 9, 7, -7, -9]: # up / down / right / left / diagonals
            square = from_square
            while True:
                square_previous = square
                square += direction
                if square < 0 or square > 63:
                    break
                if abs(square % 8 - square_previous % 8) > 1:
                    break
                target = 1 << square
                if target & allies:
                    break
                if target & enemies:
                    moves.append((from_square, square, CAPTURE))
                    break
                moves.append((from_square, square, QUIET))


def generate_king_moves(board, moves):
    color = board.turn
    king = board.bitboards[KING][color]

    targets  = 0
    targets |= (king & NOT_FILE8) << 9   # up-right
    targets |= king << 8                 # up
    targets |= (king & NOT_FILE1) << 7   # up-left
    targets |= (king & NOT_FILE8) << 1   # right
    targets |= (king & NOT_FILE1) >> 1   # left
    targets |= (king & NOT_FILE8) >> 7   # down-right
    targets |= king >> 8                 # down
    targets |= (king & NOT_FILE1) >> 9   # down-left

    # remove square where there is pieces of the same color
    targets &= ~board.get_occupied_squares(color) & FULL_BOARD

    from_square = king.bit_length() - 1

    enemies = board.get_occupied_squares(color ^ 1)

    while targets:
        lowest_bit = targets & -targets
        square = lowest_bit.bit_length() - 1
        if (1 << square) & enemies:
            moves.append((from_square, square, CAPTURE))
        else:
            moves.append((from_square, square, QUIET))
        targets &= targets - 1

if __name__ == "__main__":
    from board import Board
    board = Board()
    moves = []
    generate_pawn_moves(board, moves)
    print(moves)