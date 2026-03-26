from constants import *

class Board:

    def __init__(self, fen=STARTING_FEN):

        # bitboard
        self.pawns = [0, 0]
        self.knights = [0, 0]
        self.bishops = [0, 0]
        self.rooks = [0, 0]
        self.queens = [0, 0]
        self.kings = [0, 0]

        self.bitboards = [self.pawns, 
                          self.knights, 
                          self.bishops,
                          self.rooks,
                          self.queens,
                        self.kings
        ]

        # player to play
        self.turn = WHITE

        # castle
        self.king_castle = [False, False]
        self.queen_castle = [False, False]

        self.en_passant_square = None
        
        # fen
        self.load_fen(fen)

    def set_bit(self, color, piece, rank, file):
        square = rank * 8 + file
        self.bitboards[piece][color] |= 1 << square

    def load_fen(self, fen):
        """Load the FEN (Forsyth-Edwards Notation)"""
        parts = fen.split()
        parts += ['-'] * (6 - len(parts)) # if all parts not there
        placement, turn, castling, en_passant, halfmove, fullmove = parts

        # placement
        for ite, rank_str in enumerate(placement.split('/')):
            rank = 7 - ite
            file = 0
            for char in rank_str:
                if char.isdigit():
                    file += int(char)
                    continue
                color = WHITE if char.isupper() else BLACK
                piece = FEN_MAPPING[char.lower()]
                self.set_bit(color, piece, rank, file)
                file += 1

        self.turn = WHITE if turn == 'w' else BLACK

        if 'K' in castling: self.king_castle[WHITE] = True
        if 'Q' in castling: self.queen_castle[WHITE] = True
        if 'k' in castling: self.king_castle[BLACK] = True
        if 'q' in castling: self.queen_castle[BLACK] = True

        if en_passant != '-':
            ep_file = ord(en_passant[0]) - ord('a')
            ep_rank = int(en_passant[1]) - 1
            self.en_passant_square = ep_rank * 8 + ep_file

        if en_passant == '-':
            self.en_passant_square = None

    def __repr__(self):
        board = ''
        for rank in range(7, -1, -1): # rank 8 to 1
            board += str(rank + 1) + ' '
            for file in range(8):
                square = rank * 8 + file
                symbol = '.'
                
                for color in [WHITE, BLACK]:
                    for piece in range(6):
                        if self.bitboards[piece][color] & (1 << square):
                            symbol = PIECE_SYMBOLS[color][piece]
                board += symbol + ' '

            board += '\n'
        board += '  a b c d e f g h\n'
        return board
    
    def save_state(self):
        return (
            [bb[:] for bb in self.bitboards],
            self.en_passant_square,
            self.king_castle[:],
            self.queen_castle[:],
            self.turn,
        )

    def restore_state(self, state):
        bbs, ep, kc, qc, turn = state
        for i in range(len(self.bitboards)):
            self.bitboards[i][0] = bbs[i][0]
            self.bitboards[i][1] = bbs[i][1]
        self.en_passant_square = ep
        self.king_castle = kc
        self.queen_castle = qc
        self.turn = turn

    def get_occupied_squares(self, color):
        res = 0
        for piece in PIECES:
            res |= self.bitboards[piece][color]
        return res

    def get_empty_squares(self):
        return FULL_BOARD & ~(self.get_occupied_squares(WHITE) | self.get_occupied_squares(BLACK))
    
    def is_square_attacked(self, square, by_color):
        # pawns
        sq_bb = 1 << square
        if by_color == WHITE:
            attackers = ((sq_bb & NOT_FILE1) >> 9 | (sq_bb & NOT_FILE8) >> 7) & self.bitboards[PAWN][WHITE]
        else:
            attackers = ((sq_bb & NOT_FILE1) << 7 | (sq_bb & NOT_FILE8) << 9) & self.bitboards[PAWN][BLACK]
        if attackers:
            return True

        # knights
        knight_attacks  = 0
        knight_attacks |= (sq_bb & NOT_FILE8)             << 17
        knight_attacks |= (sq_bb & NOT_FILE8 & NOT_FILE7) << 10
        knight_attacks |= (sq_bb & NOT_FILE8 & NOT_FILE7) >> 6
        knight_attacks |= (sq_bb & NOT_FILE8)             >> 15
        knight_attacks |= (sq_bb & NOT_FILE1)             << 15
        knight_attacks |= (sq_bb & NOT_FILE1 & NOT_FILE2) << 6
        knight_attacks |= (sq_bb & NOT_FILE1 & NOT_FILE2) >> 10
        knight_attacks |= (sq_bb & NOT_FILE1)             >> 17
        if knight_attacks & self.bitboards[KNIGHT][by_color]:
            return True

        # bishops / queen (diagonals)
        for direction in [9, 7, -7, -9]:
            sq = square
            while True:
                prev_sq = sq
                sq += direction
                if sq < 0 or sq > 63:
                    break
                if abs(sq % 8 - prev_sq % 8) != 1:
                    break
                target = 1 << sq
                if target & (self.bitboards[BISHOP][by_color] | self.bitboards[QUEEN][by_color]):
                    return True
                if target & (self.get_occupied_squares(WHITE) | self.get_occupied_squares(BLACK)):
                    break  # blocked

        # rooks / queen (straight lines)
        for direction in [8, -8, 1, -1]:
            sq = square
            while True:
                prev_sq = sq
                sq += direction
                if sq < 0 or sq > 63:
                    break
                if abs(direction) == 1 and sq // 8 != prev_sq // 8:
                    break
                target = 1 << sq
                if target & (self.bitboards[ROOK][by_color] | self.bitboards[QUEEN][by_color]):
                    return True
                if target & (self.get_occupied_squares(WHITE) | self.get_occupied_squares(BLACK)):
                    break  # blocked

        # king
        king_attacks  = 0
        king_attacks |= (sq_bb & NOT_FILE8) << 9
        king_attacks |= sq_bb << 8
        king_attacks |= (sq_bb & NOT_FILE1) << 7
        king_attacks |= (sq_bb & NOT_FILE8) << 1
        king_attacks |= (sq_bb & NOT_FILE1) >> 1
        king_attacks |= (sq_bb & NOT_FILE8) >> 7
        king_attacks |= sq_bb >> 8
        king_attacks |= (sq_bb & NOT_FILE1) >> 9
        if king_attacks & self.bitboards[KING][by_color]:
            return True

        return False

    def is_in_check(self, color):
        king_sq = self.bitboards[KING][color].bit_length() - 1
        return self.is_square_attacked(king_sq, color ^ 1)
    
    def next_turn(self):
        self.turn ^= 1
    
if __name__ == "__main__":
    board = Board(fen="rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1")
    print(board)