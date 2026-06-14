#include <iostream>
#include <cmath>
#include "SimpleClock.h"
#include "MergeFinder.h"
#include "Utils.h"
#include "Generator.h"

SimpleClock clock1;


int main() {
    //double_t to_find = 9.8757731789244900612;
    double_t to_find = 8.37221162660127; //
    auto values_per_depth = Generator::initialize_values();
    MergeFinder finder(clock1, Generator::atoms());

    for (size_t depth = 2; depth <= 5; depth++) {
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
