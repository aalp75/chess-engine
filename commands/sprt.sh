#!/usr/bin/env bash

DIR="$(cd "$(dirname "$0")/.." && pwd)"

FASTCHESS="$DIR/fastchess/fastchess"

ENGINE_NEW="$DIR/engines/engine"
#ENGINE_OLD="$DIR/engines/engineOld"
STOCKFISH="$DIR/engines/stockfish-ubuntu-x86-64-avx2"

BOOK="$DIR/openings/8moves_v3.pgn"

ROUNDS=300
CONCURRENCY=19
TC="10+0.1"

LOG_DIR="./logs"
mkdir -p "$LOG_DIR"

TIMESTAMP=$(date +"%Y-%m-%d_%H-%M-%S")
LOG_FILE="$LOG_DIR/test_$TIMESTAMP.log"

echo "Starting test at $TIMESTAMP"
echo "Logging to $LOG_FILE"

#-engine cmd="$ENGINE_OLD" name=Old \

"$FASTCHESS" \
-engine cmd="$ENGINE_NEW" name=New \
-engine cmd="$STOCKFISH" name=Stockfish option.UCI_LimitStrength=true option.UCI_Elo=2575 \
-each tc=$TC \
-openings file="$BOOK" format=pgn order=random \
-rounds $ROUNDS \
-concurrency $CONCURRENCY \
| grep -E "Score|Finished|Elo|SPRT" | tee "$LOG_FILE"

echo "Finished test"