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

    /**
     * Extra constants that are seeded at depth 2 rather than depth 1, so they cost more to use (they cannot appear at
     * depth 1, and combining one is as costly as a 2-deep subtree) but stay available when genuinely needed. Extend
     * this list the same way as atoms().
     */
    static const std::vector<std::pair<double, std::string>>& constants() {
        static const std::vector<std::pair<double, std::string>> values = build_constants();
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
        std::vector<double>& result = values_per_depth[depth];
        SimpleClock clock;
        size_t previous_fill = 0;
        size_t large_half = depth - 1;
        while (large_half * 2 >= depth) { // Otherwise it's not the larger half.
            generator(values_per_depth[large_half], values_per_depth[depth - large_half], result);
            integrate_chunk(result, previous_fill, clock);
            previous_fill = result.size();
            large_half--;
        }
        // sqrt is unary and costs exactly one depth: the square roots of the depth-(d-1) values belong at depth d.
        generate_roots(values_per_depth[depth - 1], result);
        integrate_chunk(result, previous_fill, clock);
        // The extra constants are seeded once, at their home depth of 2.
        if (depth == 2) {
            previous_fill = result.size();
            seed_constants(result);
            integrate_chunk(result, previous_fill, clock);
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

    static std::vector<std::pair<double, std::string>> build_constants() {
        std::vector<std::pair<double, std::string>> constants;
        constants.emplace_back((1.0 + std::sqrt(5.0)) / 2.0, "phi"); // golden ratio
        constants.emplace_back(0.5772156649015329, "gamma");         // Euler-Mascheroni constant
        return constants;
    }

    // Appends the seeded constants to result. Called once, when result is the depth-2 tier.
    static void seed_constants(std::vector<double>& result) {
        LOG_AT(LogLevel::INFO) << "Seeding " << constants().size() << " depth-2 constants." << std::endl;
        for (const auto& [value, label] : constants()) {
            result.emplace_back(value);
        }
    }

    /**
     * Sorts the freshly appended tail [previous_fill, end) of result, merges it into the already-sorted head, and
     * prunes duplicates and non-finite values. previous_fill is result's size before the latest chunk was appended.
     */
    static void integrate_chunk(std::vector<double>& result, size_t previous_fill, SimpleClock& clock) {
        clock.start();
        std::sort(result.begin() + previous_fill, result.end()); // Don't need to sort what is already sorted.
        if (previous_fill != 0) { // For the first chunk there is nothing in front to merge with.
            std::inplace_merge(result.begin(), result.begin() + previous_fill, result.end());
        }
        LOG_AT(LogLevel::INFO) << clock.end() << " seconds used for sorting." << std::endl;
        prune_duplicates(result);
    }

    /**
     * Appends sqrt(v) for every non-negative v in source. Negative radicands would yield NaN (pruned anyway), so they
     * are skipped up front.
     * @param source values from the depth one below the target depth.
     * @param result the target depth's value vector, appended to in place.
     */
    static void generate_roots(const std::vector<double>& source, std::vector<double>& result) {
        LOG_AT(LogLevel::INFO) << "Start root generation." << std::endl;
        SimpleClock clock;
        clock.start();
        for (double v : source) {
            if (v >= 0) {
                result.emplace_back(sqrt(v));
            }
        }
        LOG_AT(LogLevel::INFO) << clock.end() << " seconds, end root generation." << std::endl;
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
        SimpleClock clock;
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
                result.emplace_back(log(a)); // sqrt is generated separately as a unary pass; natural log stays here.
            }
        }
        LOG_AT(LogLevel::INFO) << clock.end() << " seconds, end generation." << std::endl;
    }

    /**
     * This function removes all duplicate values from a sorted vector.
     * @param original values in ascending order, not containing any NaN values.
     */
    static void prune_duplicates(std::vector<double> &original) {
        SimpleClock clock;
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
};
