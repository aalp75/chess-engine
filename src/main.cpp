#include <iostream>
#include <chrono>

#include "board.h"
#include "moves.h"
#include "evaluate.h"
#include "constants.h"
#include "search.h"
#include "magic.h"

int main() {
    //std::string fen = "rnbqkbnr/pppppsppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";
    //Board board(fen);
    //board.display();
    auto start = std::chrono::high_resolution_clock::now();
    auto end = std::chrono::high_resolution_clock::now();
    magic::initMagicTables();

    std::chrono::duration<double> elapsed = end - start;
    std::cout << "Magic numbers found in " << elapsed.count() << "s\n";
}