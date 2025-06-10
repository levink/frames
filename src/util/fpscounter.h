#pragma once
#include <chrono>

struct FpsCounter {
    std::chrono::steady_clock::time_point last;
    size_t counter;

    FpsCounter();
    void print();
};
