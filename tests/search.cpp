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
#include "../src/search.h"
#include "../src/minimax.h"
#include "../src/timeManager.h"

int testAccuracy(Board& board, int depth, bool verbose) {

    SearchStats statsExpected;

    Move bestMoveExpected = findBestMoveMinimax(board, depth, statsExpected);
    int bestScoreExpected = statsExpected.score;

    SearchStats stats;
    TimeManager timeManager;
    timeManager.init(1'000'000, 0); // no time limit

    Move bestMove = findBestMove(board, depth, timeManager, stats);
    int bestScore = stats.score;

    if (verbose) {
        std::cout << "Best move : " << bestMove << " (Expected: " << bestMoveExpected << ")" << std::endl;
        std::cout << "Best score : " << bestScore << " (Expected: " << bestScoreExpected << ")" << std::endl;
    }

    return (bestScore == bestScoreExpected);
}

void testSpeed(Board& board, int depth, bool verbose) {
    SearchStats stats;
    TimeManager timeManager;
    timeManager.init(1'000'000, 0); // no time limit

    Move bestMove = findBestMove(board, depth, timeManager, stats);
    int bestScore = stats.score;
}

std::vector<std::string> readFens() {

    std::vector<std::string> fens;

    std::ifstream file("tests/perftsuite.txt");

    if (!file.is_open()) {
        std::cerr << "Could not open tests/perftsuite.txt" << std::endl;
        return fens;
    }

    std::string line;

    while (std::getline(file, line)) {
        std::istringstream ss(line);
        std::string fen, token;
        std::getline(ss, fen, ';');

        while (fen.back() == ' ') {
            fen.pop_back();
        }

        fens.push_back(fen);
    }
    return fens;
}


int main(int argc, char* argv[]) {

    std::cout.imbue(std::locale("")); // print

    std::vector<std::string> fens = readFens();

    Board board;
    StateInfo states[256];

    int passed = 0;
    int failed = 0;

    for (std::string fen : fens) {
        board.loadFen(fen);
        int res = testAccuracy(board, 5, false);
        passed += res;
        failed += (1 - res);
    }

    std::cout << "\n" << passed << " passed, " << failed << " failed\n";

    auto start = std::chrono::high_resolution_clock::now();
    for (std::string fen : fens) {
        board.loadFen(fen);
        testSpeed(board, 6, false);
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> elapsed = end - start;
    std::cout << "Tests ran in " << elapsed.count() << " seconds\n";

    return 0;
}