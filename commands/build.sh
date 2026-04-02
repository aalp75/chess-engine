#!/bin/bash
set -e

DIR="$(cd "$(dirname "$0")/.." && pwd)"
SRC_FILES=$(find "$DIR/src" -name "*.cpp")

g++ -std=c++23 -O3 -march=alderlake -mtune=alderlake -flto \
    $SRC_FILES \
    -o "$DIR/engines/engine"

echo "Build successful: $DIR/engines/engine"
