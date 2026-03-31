#pragma once

#include <chrono>

// Rename the method to better naming

struct TimeManager {
    std::chrono::steady_clock::time_point startTime;
    std::chrono::steady_clock::time_point endTime;

    void init(long long remainingMs, long long incMs);
    void initMovetime(long long movetimeMs);
    bool isExpired() const;
};  