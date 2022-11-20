#pragma once

#include <vector>
#include <cassert>
#include <iostream>
#include <cmath>
#include "SimpleClock.h"

class Generator {

public:
    static std::vector<std::vector<double>> initialize_values() {
        std::vector<std::vector<double>> result;
        result.emplace_back(std::vector<double>()); // This one stays empty because we want the vector to be 1-indexed.
        result.emplace_back(std::vector<double>());
        for (int i = 1; i <= 10; i++) {
            result[1].emplace_back(i);
        }
        return result;
    }

    static void generate_values(std::vector<std::vector<double>>& values_per_depth, size_t depth) {
        assert(values_per_depth.size() == depth);
        values_per_depth.emplace_back(std::vector<double>());
        size_t previous_fill = 0;
        size_t large_half = depth - 1;
        while (large_half * 2 >= depth) { // otherwise it's not the larger half
            generator(values_per_depth[large_half], values_per_depth[depth - large_half], values_per_depth[depth]);
            clock.start();
            std::sort(values_per_depth[depth].begin() + previous_fill, values_per_depth[depth].end()); // Don't need to sort what is already sorted
            if (previous_fill != 0) { // But in that case we need to merge the two sorted halves.
                std::inplace_merge(values_per_depth[depth].begin(), values_per_depth[depth].begin() + previous_fill,
                                   values_per_depth[depth].end());
            }
            std::cout << clock.end() << " seconds used for sorting." << std::endl;
            prune(values_per_depth[depth]);
            previous_fill = values_per_depth[depth].size();
            large_half--;
        }
    }

private:
    static void generator(const std::vector<double> &source1, const std::vector<double> &source2, std::vector<double> &result) {
        std::cout << "Start generation." << std::endl;
        clock.start();
        for (double a : source1) {
            for (double b : source2) {
                result.emplace_back(a + b);
                result.emplace_back(a - b);
                result.emplace_back(b - a);
                result.emplace_back(a * b);
                result.emplace_back(a / b);
                result.emplace_back(b / a);
                if (a > 0) { // For negative values of a, this can lead to NaNs. In general not much reason to use these type of values.
                    result.emplace_back(pow(a, b));
                }
                if (b > 0) {
                    result.emplace_back(pow(b, a));
                }
            }
            if (a > 0) {
                result.emplace_back(sqrt(a));
                result.emplace_back(log(a));
            }
        }
        std::cout << clock.end() << " seconds, end generation." << std::endl;
    }

    static void prune(std::vector<double> &original) {
        clock.start();
        std::cout << "Start pruning." << std::endl;
        std::vector<double> pruned;
        for (double & i : original) {
            if (pruned.empty() || i != pruned.back()) {
                if (std::isfinite(i)) {
                    pruned.emplace_back(i);
                }
            }
        }
        std::cout << clock.end() << " seconds, pruned " << original.size() << " to " << pruned.size() << std::endl;
        original.swap(pruned);
    }

    static SimpleClock clock;
};