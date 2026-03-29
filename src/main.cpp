#include<iostream>
#include<vector>

#include "board.h"
#include "moves.h"
#include "evaluate.h"
#include "constants.h"
#include "search.h"

int main() {
    std::string fen = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";
    Board board = Board(fen);
    board.display();
    

    std::vector<std::vector<int>> moves = generateMoves(board);
    std::cout << moves.size() << " moves in total\n";
    for (auto move : moves) {
        //std::cout << move[0] << " " << move[1] << " " << move[2] << " " << move[3] << std::endl;
    }

    std::cout << "Finished to run" << std::endl;
}