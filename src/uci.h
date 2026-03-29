#pragma once

#include <string>
#include <vector>

#include "board.h"

std::string sqToStr(int sq);
std::string moveToUci(const Move move);

void run(int depth);



