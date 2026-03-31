#!/bin/bash
set -e

ROOT=/home/antoine/Documents/github/chess-engine

g++ -std=c++17 -O2 \
    "$ROOT/tests/perft.cpp" \
    "$ROOT/src/board.cpp" \
    "$ROOT/src/moves.cpp" \
    "$ROOT/src/magic.cpp" \
    -I "$ROOT/src" \
    -o "$ROOT/tests/perft_test"

"$ROOT/tests/perft_test"
