#!/bin/bash
set -e

ROOT=/home/antoine/Documents/github/chess-engine

g++ -std=c++23 -O3 -march=alderlake -mtune=alderlake -flto \
    "$ROOT/tests/perft.cpp" \
    "$ROOT/src/board.cpp" \
    "$ROOT/src/moves.cpp" \
    "$ROOT/src/magic.cpp" \
    "$ROOT/src/utils.cpp" \
    "$ROOT/src/zobrist.cpp" \
    "$ROOT/src/evaluate.cpp" \
    -I "$ROOT/src" \
    -o "$ROOT/tests/perft_test"

"$ROOT/tests/perft_test"
