#include "fpscounter.h"
#include <iostream>

using std::chrono::microseconds;
using std::chrono::steady_clock;
using std::chrono::duration_cast;

FpsCounter::FpsCounter() {
   
    last = steady_clock::now();
    counter = 0;
}
void FpsCounter::print() {
    counter++;

    auto now = steady_clock::now();
    auto delta = duration_cast<microseconds>(now - last).count();
    if (delta > 1000000) {
        std::cout << "fps=" << counter << std::endl;
        last = now;
        counter = 0;
    }
}