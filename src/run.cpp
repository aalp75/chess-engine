#include "uci.h"
#include "search.h"
#include "moves.h"
#include "board.h"

int main() {
    int maxDepth = 50;
    bool playSound = true;
    run(maxDepth, playSound);
}