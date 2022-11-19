#ifndef FORMULAFINDER_SIMPLECLOCK_H
#define FORMULAFINDER_SIMPLECLOCK_H

#include <chrono>
#include <iostream>

class SimpleClock {

private:
    std::chrono::time_point<std::chrono::system_clock> start_time = std::chrono::system_clock::now();

public:
    void start() {
        start_time = std::chrono::system_clock::now();
    }

    double end() {
        auto end_time = std::chrono::system_clock::now();
        std::chrono::duration<double> elapsed_seconds = end_time - start_time;
        return elapsed_seconds.count();
    }
};


#endif //FORMULAFINDER_SIMPLECLOCK_H
