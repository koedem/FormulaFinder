#include <iostream>
#include <vector>
#include <cassert>
#include "math.h"
#include "SimpleClock.h"
#include "MergeFinder.h"
#include "Utils.h"

SimpleClock clock1;



void generator(const std::vector<double> &source1, const std::vector<double> &source2, std::vector<double> &result) {
    std::cout << "Start generation." << std::endl;
    clock1.start();
    for (double a : source1) {
        for (double b : source2) {
            result.emplace_back(a + b);
            result.emplace_back(a - b);
            result.emplace_back(b - a);
            result.emplace_back(a * b);
            result.emplace_back(a / b);
            result.emplace_back(b / a);
            result.emplace_back(pow(a,b));
            result.emplace_back(pow(b,a));
            //result.emplace_back(log(a) / log(b));
            //result.emplace_back(log(b) / log(a));
        }
        result.emplace_back(sqrt(a)); // could also be done by pow 1/2 but that costs an extra depth
        result.emplace_back(log(a));
    }
    std::cout << clock1.end() << " seconds, end generation." << std::endl;
}

void prune(std::vector<double> &original) {
    clock1.start();
    std::cout << "Start pruning." << std::endl;
    std::vector<double> pruned;
    for (double & i : original) {
        if (pruned.empty() || i != pruned.back()) {
            if (isfinite(i)) {
                pruned.emplace_back(i);
            }
        }
    }
    std::cout << clock1.end() << " seconds, pruned " << original.size() << " to " << pruned.size() << std::endl;
    original.swap(pruned);
}

std::vector<std::vector<double>> initialize_values() {
    std::vector<std::vector<double>> result;
    result.emplace_back(std::vector<double>()); // This one stays empty because we want the vector to be 1-indexed.
    result.emplace_back(std::vector<double>());
    for (int i = 1; i <= 10; i++) {
        result[1].emplace_back(i);
    }
    return result;
}

void generate_values(std::vector<std::vector<double>>& values_per_depth, size_t depth) {
    assert(values_per_depth.size() == depth);
    values_per_depth.emplace_back(std::vector<double>());
    size_t large_half = depth - 1;
    while (large_half * 2 >= depth) { // otherwise it's not the larger half
        generator(values_per_depth[large_half], values_per_depth[depth - large_half], values_per_depth[depth]);
        std::sort(values_per_depth[depth].begin(), values_per_depth[depth].end());
        prune(values_per_depth[depth]);
        large_half--;
    }
}

void count_matching_negative_numbers(const std::vector<double>& values) {
    long count = 0, notEqual = 0;
    for (double a : values) {
        if (a < 0) {
            double x = -a;
            size_t index = Utils::approxBinSearch(values, x); {
                if (values[index] == x) {
                    count++;
                } else {
                    notEqual++;
                }
            }
        }
    }
    std::cout << "Of the " << count << " negative numbers, " << notEqual << " are not found among the positives." << std::endl;
}

int main() {
    double_t to_find = 9.8757731789244900612;
    auto values_per_depth = initialize_values();
    MergeFinder finder(clock1);

    for (size_t depth = 2; depth <= 6; depth++) {
        finder.findAndPrint<true>(depth, values_per_depth, to_find);
        std::cout << std::endl;

        generate_values(values_per_depth, depth);
        count_matching_negative_numbers(values_per_depth[depth]);
    }

    finder.findAndPrint<true>(7, values_per_depth, to_find);
    std::cout << std::endl;
    finder.findAndPrint<true>(8, values_per_depth, to_find);
    std::cout << std::endl;
    return 0;
}