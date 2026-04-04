#include <chrono>
#include <cmath>

#include "timeManager.h"

void TimeManager::init(long long remainingMs, long long incMs) {
    startTime = std::chrono::steady_clock::now();
    long long budget = remainingMs / 20 + incMs / 2;
    /*if (remainingMs < 10000) { // under 10 seconds
        budget = remainingMs / 30 + incMs / 3;
    }*/
    endTime = startTime + std::chrono::milliseconds(budget);
}

void TimeManager::initMovetime(long long movetimeMs) {
    startTime = std::chrono::steady_clock::now();
    endTime = startTime + std::chrono::milliseconds(movetimeMs);
}

bool TimeManager::isExpired() const {
    return (std::chrono::steady_clock::now() > endTime);
}