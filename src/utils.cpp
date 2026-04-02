#include <random>

#include "utils.h"

std::mt19937_64 rng(57);

uint64_t random64() {
    return rng();
}