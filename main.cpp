#include <iostream>
#include <vector>
#include <cassert>
#include <cmath>
#include "SimpleClock.h"
#include "MergeFinder.h"
#include "Utils.h"
#include "Generator.h"

SimpleClock clock1;


int main() {
    double_t to_find = 9.8757731789244900612;
    auto values_per_depth = Generator::initialize_values();
    MergeFinder finder(clock1);

    for (size_t depth = 2; depth <= 6; depth++) {
        finder.findAndPrint<true>(depth, values_per_depth, to_find);
        std::cout << std::endl;

        Generator::generate_values(values_per_depth, depth);
        Utils::count_matching_negative_numbers(values_per_depth[depth]);
    }

    finder.findAndPrint<true>(7, values_per_depth, to_find);
    std::cout << std::endl;
    finder.findAndPrint<true>(8, values_per_depth, to_find);
    std::cout << std::endl;
    return 0;
}
