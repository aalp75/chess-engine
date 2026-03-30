#!/bin/bash
set -e

DIR="$(cd "$(dirname "$0")/.." && pwd)"

g++ -std=c++23 -O3 -march=alderlake -mtune=alderlake -flto \
    "$DIR/src/run.cpp" \
    "$DIR/src/board.cpp" \
    "$DIR/src/moves.cpp" \
    "$DIR/src/evaluate.cpp" \
    "$DIR/src/search.cpp" \
    "$DIR/src/uci.cpp" \
    "$DIR/src/transpositionTable.cpp" \
    "$DIR/src/magic.cpp" \
    -o "$DIR/engine"

echo "Build successful: $DIR/engine"
