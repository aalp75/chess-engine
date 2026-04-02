#include <chrono>
#include <cmath>

#include "timeManager.h"

void TimeManager::init(long long remainingMs, long long incMs) {
    startTime = std::chrono::steady_clock::now();
    long long budget = remainingMs / 22 + incMs / 3;
    long long cap = remainingMs / 4;
    if (remainingMs < 10000) { // under 10 seconds
        budget = std::min(budget, remainingMs / 8);
    }
    endTime = startTime + std::chrono::milliseconds(std::min(budget, cap));
}

void TimeManager::initMovetime(long long movetimeMs) {
    startTime = std::chrono::steady_clock::now();
    endTime = startTime + std::chrono::milliseconds(movetimeMs);
}

bool TimeManager::isExpired() const {
    return (std::chrono::steady_clock::now() > endTime) ? true : false;
}