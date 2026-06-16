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
        // Size the tier once, exactly: no reallocation while generating, and since the reserved-but-unwritten tail
        // never faults in, resident memory stays bounded by the live element count rather than the reservation.
        result.reserve(planned_output_size(values_per_depth, depth));
        SimpleClock clock;
        size_t large_half = depth - 1;
        while (large_half * 2 >= depth) { // Otherwise it's not the larger half.
            generator(values_per_depth[large_half], values_per_depth[depth - large_half], result);
            integrate_chunk(result, clock);
            large_half--;
        }
        // sqrt is unary and costs exactly one depth: the square roots of the depth-(d-1) values belong at depth d.
        generate_roots(values_per_depth[depth - 1], result);
        integrate_chunk(result, clock);
        // The extra constants are seeded once, at their home depth of 2.
        if (depth == 2) {
            seed_constants(result);
            integrate_chunk(result, clock);
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
     * Sorts the whole tier and prunes its duplicates and non-finite values. The freshly appended chunk could be
     * sorted alone and merged into the already-sorted head, but std::inplace_merge allocates an auxiliary buffer the
     * size of the merged range; sorting the whole tier in place (introsort allocates nothing) trades some extra
     * sorting work for a markedly smaller peak footprint.
     */
    static void integrate_chunk(std::vector<double>& result, SimpleClock& clock) {
        clock.start();
        std::sort(result.begin(), result.end());
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

    static size_t count_positive(const std::vector<double>& v) { // strictly > 0; v is sorted ascending.
        return static_cast<size_t>(v.end() - std::upper_bound(v.begin(), v.end(), 0.0));
    }

    static size_t count_nonneg(const std::vector<double>& v) {    // >= 0; v is sorted ascending.
        return static_cast<size_t>(v.end() - std::lower_bound(v.begin(), v.end(), 0.0));
    }

    // Exactly how many values generator(s1, s2, ...) appends: six results per pair, plus pow(a, b) for each a > 0,
    // pow(b, a) for each b > 0, and one log(a) per a > 0. Mirrors generator's branches.
    static size_t generator_output_size(const std::vector<double>& s1, const std::vector<double>& s2) {
        const size_t n1 = s1.size(), n2 = s2.size();
        const size_t pos1 = count_positive(s1), pos2 = count_positive(s2);
        return 6 * n1 * n2 + pos1 * n2 + n1 * pos2 + pos1;
    }

    // Total number of values generate_values will append for this depth, before any pruning -- the exact capacity
    // the result tier needs. Mirrors the chunk sequence of generate_values (binary combinations, then the unary
    // sqrt pass, then the seeded constants at depth 2), so the two must be kept in step.
    static size_t planned_output_size(const std::vector<std::vector<double>>& values_per_depth, size_t depth) {
        size_t total = 0;
        size_t large_half = depth - 1;
        while (large_half * 2 >= depth) {
            total += generator_output_size(values_per_depth[large_half], values_per_depth[depth - large_half]);
            large_half--;
        }
        total += count_nonneg(values_per_depth[depth - 1]); // the unary sqrt pass.
        if (depth == 2) { total += constants().size(); }    // the seeded constants.
        return total;
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
     * This function removes all duplicate and non-finite values from a sorted vector, in place. A write cursor
     * compacts the survivors toward the front and the tail is dropped with resize, so no second buffer is allocated
     * (the capacity is retained for the next chunk). Equivalent to keeping each value that is finite and differs
     * from the previous survivor.
     * @param values in ascending order, not containing any NaN values.
     */
    static void prune_duplicates(std::vector<double> &values) {
        SimpleClock clock;
        clock.start();
        LOG_AT(LogLevel::INFO) << "Start pruning." << std::endl;
        const size_t before = values.size();
        size_t write = 0;
        for (size_t read = 0; read < values.size(); read++) {
            const double v = values[read];
            if (std::isfinite(v) && (write == 0 || v != values[write - 1])) {
                values[write++] = v;
            }
        }
        values.resize(write); // drops the pruned tail without releasing capacity.
        LOG_AT(LogLevel::INFO) << clock.end() << " seconds, pruned " << before << " to " << values.size() << std::endl;
    }
};
