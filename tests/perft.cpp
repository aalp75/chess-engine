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
    int turn = board.turn;
    
    for (int i = 0; i < moves.count; i++) {
        Move move = moves.moves[i];
        doMove(board, move, states, ply);
        //assert(board.key == board.hash());
        if (!board.isInCheck(board.turn ^ 1)) {
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

/*int main() {
    std::string fen = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";
    Board board(fen);
    StateInfo states[256];
    std::vector<int> expected = {1, 20, 400, 8902, 197281, 4865609, 119060324};

    for (int depth = 0; depth <= 3; depth++) {
        int nodes = perft(board, depth, 0, states);
        std::cout.imbue(std::locale(""));
        std::string verdict = (nodes == expected[depth]) ? "OK" : "FAILED";
        std::cout << "Depth = " << depth << " Nodes = " << nodes;
        std::cout <<" (Expected = " << expected[depth] << ") - " << verdict << std::endl;
    }

    fen = "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1";
    board = Board(fen);

    expected = {1, 48, 2039, 97862, 4085603, 193690690};

    for (int depth = 0; depth <= 3; depth++) {
        int nodes = perft(board, depth, 0, states);
        std::cout.imbue(std::locale(""));
        std::string verdict = (nodes == expected[depth]) ? "OK" : "FAILED";
        std::cout << "Depth = " << depth << " Nodes = " << nodes;
        std::cout <<" (Expected = " << expected[depth] << ") - " << verdict << std::endl;
    }

    fen = "n1n5/PPPk4/8/8/8/8/4Kppp/5N1N b - - 0 1";
    board = Board(fen);

    expected = {1, 24, 496, 9483, 182838, 3605103};

    for (int depth = 0; depth <= 5; depth++) {
        int nodes = perft(board, depth, 0, states);
        std::cout.imbue(std::locale(""));
        std::string verdict = (nodes == expected[depth]) ? "OK" : "FAILED";
        std::cout << "Depth = " << depth << " Nodes = " << nodes;
        std::cout <<" (Expected = " << expected[depth] << ") - " << verdict << std::endl;
    }


}*/

int main(int argc, char* argv[]) {

    int maxDepth = 5;
    if (argc > 1) {
        maxDepth = std::stoi(argv[1]);
    }

    std::cout.imbue(std::locale("")); // print

    std::ifstream file("tests/perftsuite.txt");

    if (!file.is_open()) {
        std::cerr << "Could not open tests/perftsuite.txt" << std::endl;
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