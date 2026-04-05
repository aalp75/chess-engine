#!/bin/bash
set -e

ROOT=/home/antoine/Documents/github/chess-engine

g++ -std=c++23 -O3 -march=alderlake -mtune=alderlake -flto \
    "$ROOT/tests/search.cpp" \
    "$ROOT/src/board.cpp" \
    "$ROOT/src/moves.cpp" \
    "$ROOT/src/magic.cpp" \
    "$ROOT/src/utils.cpp" \
    "$ROOT/src/zobrist.cpp" \
    "$ROOT/src/minimax.cpp" \
    "$ROOT/src/evaluate.cpp" \
    "$ROOT/src/search.cpp" \
    "$ROOT/src/searchNew.cpp" \
    "$ROOT/src/timeManager.cpp" \
    "$ROOT/src/transpositionTable.cpp" \
    -I "$ROOT/src" \
    -o "$ROOT/tests/search_test"

"$ROOT/tests/search_test"
