#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <locale>
#include <chrono>
#include <cassert>
#include <cstring>

#include "../src/board.h"
#include "../src/moves.h"
#include "../src/constants.h"
#include "../src/moveList.h"
#include "../src/search.h"
#include "../src/minimax.h"
#include "../src/timeManager.h"
#include "../src/transpositionTable.h"
#include "../src/utils.h"


bool testAccuracy(std::vector<std::string> fens, int maxDepth, bool verbose) {

    int passed = 0;
    int failed = 0;

    Board board;

    for (int depth = 1; depth <= maxDepth; depth++) {
        for (std::string fen : fens) {
            board.loadFen(fen);
            clearTT();

            SearchStats statsExpected;

            Move bestMoveExpected = findBestMoveMinimax(board, depth, statsExpected);
            int bestScoreExpected = statsExpected.score;

            SearchStats stats;
            TimeManager timeManager;
            timeManager.init(1'000'000, 0); // no time limit

            Move bestMove = findBestMove(board, depth, timeManager, stats, false);
            int bestScore = stats.score;

            if (verbose) {
                std::cout << "Best move : " << bestMove << " (Expected: " << bestMoveExpected << ")" << std::endl;
                std::cout << "Best score : " << bestScore << " (Expected: " << bestScoreExpected << ")" << std::endl;
            }

            int res = (bestScore == bestScoreExpected);
            passed += res;
            failed += (1 - res);
        }
    }

    std::cout << "\n" << passed << " passed, " << failed << " failed\n";

    return (failed == 0);
}

SearchStats testSpeed(Board& board, int depth, bool verbose) {
    SearchStats stats;
    TimeManager timeManager;
    timeManager.init(1'000'000, 0); // no time limit
    clearTT();
    findBestMove(board, depth, timeManager, stats, true);
    return stats;
}

std::vector<std::string> readFens() {

    std::vector<std::string> fens;

    std::ifstream file("tests/inputs/perftsuite.epd");

    if (!file.is_open()) {
        std::cerr << "Could not open tests/inputs/perftsuite.epd" << std::endl;
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

    file.close();

    std::string path = "tests/inputs/wac.epd";
    file.open(path);
    if (!file.is_open()) {
        std::cerr << "Could not open " << path << std::endl;
        return fens;
    }

    while (std::getline(file, line)) {
        std::istringstream ss(line);
        std::string token, fen;
        for (int i = 0; i < 6 && ss >> token; i++) {
            if (i > 0) fen += " ";
            fen += token;
        }
        if (!fen.empty()) fens.push_back(fen);
    }
    return fens;
}

int main(int argc, char* argv[]) {

    std::cout.imbue(std::locale("")); // print

    std::vector<std::string> fens = readFens();

    Board board;

    //int maxDepth = 2;
    //testAccuracy(fens, maxDepth, false);

    auto start = std::chrono::high_resolution_clock::now();
    int depth = 10;
    int cnt = 0;
    long long totalNNodes = 0;
    long long totalQNodes = 0;
    long long totalTime = 0;
    int seldepth = 0;
    for (std::string fen : fens) {
        board.loadFen(fen);
        SearchStats s = testSpeed(board, depth, false);
        totalNNodes += s.nodes;
        totalQNodes += s.qnodes;
        seldepth = std::max(seldepth, s.seldepth);
        cnt++;
        if (cnt > 10) {
            break;
        }
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> elapsed = end - start;
    std::cout << "Results: " << std::endl;
    std::cout << " - Depth: " << depth << " (deepest: " << seldepth << ")" << std::endl;
    std::cout << " - Speed: " << elapsed.count() << " seconds" << std::endl;
    std::cout << " - Nodes: " << formatNumber(totalNNodes) << " seconds" << std::endl;
    std::cout << " - Qnodes: " << formatNumber(totalQNodes) << " seconds" << std::endl;

    long long nps = (totalNNodes + totalQNodes) / elapsed.count();

    std::cout << " - NPS: " << formatNumber(nps) << "\n";

    return 0;
}