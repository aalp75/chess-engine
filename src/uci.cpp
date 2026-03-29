#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <algorithm>
#include <unordered_map>
#include <chrono>
#include <iomanip>
#include <ctime>
#include <filesystem>

#include "uci.h"
#include "search.h"
#include "moves.h"
#include "board.h"
#include "evaluate.h"
#include "constants.h"
#include "moveList.h"

/*
    TODO: Add a small array int[] infos that keeps all the important infos for debugging:
    score, depth, max_depth, nodes, tt look up...

    TODO: add a try except also to print in log
*/

static std::ofstream logFile;

static void logMsg(const std::string& dir, const std::string& msg) {
    if (logFile.is_open()) {
        logFile << dir << " " << msg << "\n";
        logFile.flush();
    }
}

static void send(const std::string& msg) {
    logMsg(">", msg);
    std::cout << msg << "\n";
}

static const std::unordered_map<char, int> PROMO_CHAR = {
    {'q', QUEEN},
    {'r', ROOK},
    {'b', BISHOP},
    {'n', KNIGHT}
};

static const std::unordered_map<int, char> PROMO_CHAR_REV = {
    {QUEEN, 'q'},
    {ROOK, 'r'},
    {BISHOP, 'b'},
    {KNIGHT, 'n'}
};

std::string sqToStr(int sq) {
    char file = 'a' + (sq % 8);
    char rank = '1' + (sq / 8);
    return std::string{file, rank};
}

std::string moveToUci(const Move move) {
    int from = moveFrom(move);
    int to = moveTo(move);
    int type = moveType(move);
    int promo = movePromo(move);

    std::string s = sqToStr(from) + sqToStr(to);

    if (type == PROMOTION) {
        auto it = PROMO_CHAR_REV.find(promo);
        if (it != PROMO_CHAR_REV.end()) {
            s += it->second;
        }
    }

    return s;
}

Move uciToMove(Board& board, const std::string& uci) {
    MoveList moves = generateMoves(board);

    for (int i = 0; i < moves.count; i++) {
        int move = moves.moves[i];
        if (moveToUci(move) == uci) {
            return move;
        }
    }

    return {};
}

std::vector<std::string> split(const std::string& s) {
    std::istringstream iss(s);
    std::vector<std::string> tokens;
    std::string token;

    while (iss >> token) {
        tokens.push_back(token);
    }

    return tokens;
}

void applyMoves(Board& board, const std::vector<std::string>& tokens) {
    StateInfo states[256];
    int ply = 0;

    for (const std::string& uci : tokens) {
        Move move = uciToMove(board, uci);
        if (move != 0) {
            doMove(board, move, states, ply++);
        }
    }
}

void run(int initialDepth) {
    const std::string logDir = "/home/antoine/Documents/github/chess-engine/logs";
    std::filesystem::create_directories(logDir);
    auto now = std::chrono::system_clock::now();
    std::time_t t = std::chrono::system_clock::to_time_t(now);
    std::ostringstream logName;
    logName << logDir << "/" << std::put_time(std::localtime(&t), "%Y%m%d_%H%M%S") << ".log";
    logFile.open(logName.str());

    std::string startFen = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";
    Board board(startFen);

    std::string line;
    while (std::getline(std::cin, line)) {
        logMsg("<", line);

        std::vector<std::string> tokens = split(line);
        if (tokens.empty()) continue;

        if (tokens[0] == "uci") {
            send("id name Toti");
            send("id author aalp75");
            send("option name Move Overhead type spin default 10 min 0 max 5000");
            send("option name Threads type spin default 1 min 1 max 1");
            send("uciok");

        } else if (tokens[0] == "isready") {
            send("readyok");

        } else if (tokens[0] == "setoption") {
            // not implemented for now

        } else if (tokens[0] == "ucinewgame") {
            board = Board(startFen);

        } else if (tokens[0] == "position") {
            if (tokens.size() >= 2 && tokens[1] == "startpos") {
                board = Board(startFen);

                auto it = std::find(tokens.begin(), tokens.end(), "moves");
                if (it != tokens.end()) {
                    std::vector<std::string> moves(it + 1, tokens.end());
                    applyMoves(board, moves);
                }
            } else if (tokens.size() >= 3 && tokens[1] == "fen") {
                auto it = std::find(tokens.begin(), tokens.end(), "moves");
                size_t fenEnd = (it != tokens.end())
                    ? std::distance(tokens.begin(), it)
                    : tokens.size();

                std::string fen;
                for (size_t i = 2; i < fenEnd; i++) {
                    if (i > 2) fen += " ";
                    fen += tokens[i];
                }

                board = Board(fen);

                if (it != tokens.end()) {
                    std::vector<std::string> moves(it + 1, tokens.end());
                    applyMoves(board, moves);
                }
            }
        }
        else if (tokens[0] == "go") {
            int depth = initialDepth;

            auto itDepth = std::find(tokens.begin(), tokens.end(), "depth");
            if (itDepth != tokens.end() && itDepth + 1 != tokens.end()) {
                depth = std::stoi(*(itDepth + 1));
            } 
            else {
                auto itMoveTime = std::find(tokens.begin(), tokens.end(), "movetime");
                if (itMoveTime != tokens.end() && itMoveTime + 1 != tokens.end()) {
                    int movetime = std::stoi(*(itMoveTime + 1));
                    if (movetime < 3000) depth--;
                }

                std::string timeToken = (board.turn == WHITE) ? "wtime" : "btime";
                auto itTime = std::find(tokens.begin(), tokens.end(), timeToken);
                if (itTime != tokens.end() && itTime + 1 != tokens.end()) {
                    int remainingTime = std::stoi(*(itTime + 1));
                    if (remainingTime < 60000) depth--;
                }
            }

            int score = 0;
            long long nodes = 0;
            Move move = findBestMove(board, depth, score, nodes);

            if (move != 0) {
                std::string uci = moveToUci(move);
                send("info depth " + std::to_string(depth) + " score cp " + std::to_string(score) + " nodes " + std::to_string(nodes) + " pv " + uci);
                send("bestmove " + uci);
            } else {
                send("info depth " + std::to_string(depth) + " score cp " + std::to_string(score) + " nodes " + std::to_string(nodes));
                send("bestmove 0000");
            }

        } else if (tokens[0] == "stop") {
            // nothing to do

        } else if (tokens[0] == "quit") {
            break;
        }

        std::cout.flush();
    }

    logFile.close();
}
