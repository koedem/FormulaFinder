#pragma once

#include <vector>
#include <cassert>
#include <iostream>
#include <cmath>
#include <algorithm>
#include <string>
#include <utility>
#include "SimpleClock.h"
#include "Log.h"

class Generator {

public:
    /**
     * Single source of truth for the depth-1 atoms: each is a numeric value paired with the symbol printed for it.
     * Adding constants (e.g. {std::sqrt(2.0), "sqrt2"}) only requires extending this list.
     */
    static const std::vector<std::pair<double, std::string>>& atoms() {
        static const std::vector<std::pair<double, std::string>> values = build_atoms();
        return values;
    }

    static std::vector<std::vector<double>> initialize_values() {
        std::vector<std::vector<double>> result;
        result.emplace_back(std::vector<double>()); // This one stays empty because we want the vector to be 1-indexed.
        result.emplace_back(std::vector<double>());
        for (const auto& [value, label] : atoms()) {
            result[1].emplace_back(value);
        }
        std::sort(result[1].begin(), result[1].end()); // All searches assume each depth's values are sorted ascending.
        return result;
    }

    static void generate_values(std::vector<std::vector<double>>& values_per_depth, size_t depth) {
        assert(values_per_depth.size() == depth);
        values_per_depth.emplace_back(std::vector<double>());
        size_t previous_fill = 0;
        size_t large_half = depth - 1;
        while (large_half * 2 >= depth) { // Otherwise it's not the larger half.
            generator(values_per_depth[large_half], values_per_depth[depth - large_half], values_per_depth[depth]);
            clock.start();
            std::sort(values_per_depth[depth].begin() + previous_fill, values_per_depth[depth].end()); // Don't need to sort what is already sorted.
            if (previous_fill != 0) { // But in that case we need to merge the two sorted halves.
                std::inplace_merge(values_per_depth[depth].begin(), values_per_depth[depth].begin() + previous_fill,
                                   values_per_depth[depth].end());
            }
            LOG_AT(LogLevel::INFO) << clock.end() << " seconds used for sorting." << std::endl;
            prune_duplicates(values_per_depth[depth]);
            previous_fill = values_per_depth[depth].size();
            large_half--;
        }
    }

private:
    static std::vector<std::pair<double, std::string>> build_atoms() {
        std::vector<std::pair<double, std::string>> atoms;
        for (int i = 1; i <= 8; i++) {
            atoms.emplace_back(static_cast<double>(i), std::to_string(i));
        }
        atoms.emplace_back(M_PI, "pi");
        atoms.emplace_back(M_E, "e");
        return atoms;
    }

    /**
     * This function combines the values of the two inputs through arithmetic operations. These arithmetic operations
     * include sum, product, exponentiation and more.
     * @param source1 containing no infinite or NaN values.
     * @param source2 containing no infinite or NaN values.
     * @param result all results for OP(x, y) for x in source1, y in source2.
     */
    static void generator(const std::vector<double> &source1, const std::vector<double> &source2, std::vector<double> &result) {
        LOG_AT(LogLevel::INFO) << "Start generation." << std::endl;
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
        LOG_AT(LogLevel::INFO) << clock.end() << " seconds, end generation." << std::endl;
    }

    /**
     * This function removes all duplicate values from a sorted vector.
     * @param original values in ascending order, not containing any NaN values.
     */
    static void prune_duplicates(std::vector<double> &original) {
        clock.start();
        LOG_AT(LogLevel::INFO) << "Start pruning." << std::endl;
        std::vector<double> pruned;
        for (double & i : original) {
            if (pruned.empty() || i != pruned.back()) {
                if (std::isfinite(i)) {
                    pruned.emplace_back(i);
                }
            }
        }
        LOG_AT(LogLevel::INFO) << clock.end() << " seconds, pruned " << original.size() << " to " << pruned.size() << std::endl;
        original.swap(pruned);
    }

    inline static SimpleClock clock;
};
