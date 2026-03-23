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

        self.en_passant_square = 0
        
        # fen
        self.load_fen(fen)

    def set_bit(self, color, piece, rank, file):
        square = rank * 8 + file
        self.bitboards[piece][color] |= 1 << square

    def load_fen(self, fen):
        """Load the FEN (Forsyth-Edwards Notation)"""
        parts = fen.split()
        placement, turn, castling, en_passant, halfmove, fullmove = parts

        print("placement", placement)
        print("turn:", turn)
        print("castling:", castling)
        print("en_passant:", en_passant)
        print("halfmove:", halfmove)
        print("fullmove:", fullmove)

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
    
    def get_occupied_squares(self, color):
        res = 0
        for piece in PIECES:
            res |= self.bitboards[piece][color]
        return res

    def get_empty_squares(self):
        return FULL_BOARD & ~(self.get_occupied_squares(WHITE) | self.get_occupied_squares(BLACK))
    
if __name__ == "__main__":
    board = Board(fen="rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1")
    print(board)