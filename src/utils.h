#pragma once

uint64_t random64();

template <typename T>
int popcount(T t) {
    return __builtin_popcountll(static_cast<uint64_t>(t));
}

template <typename T>
int lsb(T t) {
    return __builtin_ctzll(static_cast<uint64_t>(t));
}