#!/bin/bash
set -e

ROOT=/home/antoine/Documents/github/chess-engine

g++ -std=c++17 -O2 -DTEST\
    "$ROOT/tests/search.cpp" \
    "$ROOT/src/board.cpp" \
    "$ROOT/src/moves.cpp" \
    "$ROOT/src/magic.cpp" \
    "$ROOT/src/utils.cpp" \
    "$ROOT/src/zobrist.cpp" \
    "$ROOT/src/minimax.cpp" \
    "$ROOT/src/evaluate.cpp" \
    "$ROOT/src/search_new.cpp" \
    "$ROOT/src/timeManager.cpp" \
    -I "$ROOT/src" \
    -o "$ROOT/tests/search_test"

"$ROOT/tests/search_test"
