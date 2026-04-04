#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <locale>
#include <chrono>
#include <cassert>

#include "../src/board.h"
#include "../src/moves.h"
#include "../src/constants.h"
#include "../src/moveList.h"

int perft(Board& board, int depth, int ply, StateInfo* states) {
    if (depth == 0) return 1;
    
    MoveList moves = generateMoves(board);
    int totalNodes = 0;
    int side = board.side;
    
    for (int i = 0; i < moves.count; i++) {
        Move move = moves.moves[i];
        doMove(board, move, states, ply);
        //assert(board.key == board.hash());
        if (!board.isInCheck(board.side ^ 1)) {
            int nodes = perft(board, depth - 1, ply + 1, states);
            totalNodes += nodes;
            if (ply == 0) {
                //std::cout << move[0] << "->" << move[1] << " : " << nodes << std::endl;
            }
        }
        undoMove(board, states, ply);
    }

    return totalNodes;
}

int main(int argc, char* argv[]) {

    int maxDepth = 5;
    if (argc > 1) {
        maxDepth = std::stoi(argv[1]);
    }

    std::cout.imbue(std::locale("")); // print

    std::ifstream file("tests/inputs/perftsuite.epd");

    if (!file.is_open()) {
        std::cerr << "Could not open tests/inputs/perftsuite.epd" << std::endl;
        return 1;
    }

    Board board;
    StateInfo states[256];
    int passed = 0;
    int failed = 0;

    auto start = std::chrono::high_resolution_clock::now();

    std::string line;

    while (std::getline(file, line)) {
        std::istringstream ss(line);
        std::string fen, token;
        std::getline(ss, fen, ';');

        while (fen.back() == ' ') {
            fen.pop_back();
        }

        std::cout << "Run test for fen " << fen << std::endl;
        board.loadFen(fen);
        
        while (std::getline(ss, token, ';')) {
            if (token.size() < 3 || token[0] != 'D') continue;
            int depth = std::stoi(token.substr(1));

            long long expected = std::stoll(token.substr(token.find(' ') + 1));

            if (depth > maxDepth) {
                continue;
            }

            long long nodes = perft(board, depth, 0, states);
            bool ok = (nodes == expected);

            ok ? passed++ : failed++;

            std::cout << (ok ? "OK  " : "FAIL") << "  Depth = " << depth
                      << "  Nodes = " << nodes << " (Expected = " << expected << ")\n";
        }
    }

    std::cout << "\n" << passed << " passed, " << failed << " failed\n";
    
    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> elapsed = end - start;
    std::cout << "Tests ran in " << elapsed.count() << " seconds\n";

    return failed > 0 ? 1 : 0;
}