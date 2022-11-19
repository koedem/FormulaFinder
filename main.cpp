#include <iostream>
#include <vector>
#include <cassert>
#include "math.h"
#include "SimpleClock.h"

SimpleClock clock1;



unsigned long approxBinSearch(const std::vector<double> &sorted, double to_find) {
    if (to_find < sorted[0]) {
        return 0;
    } else if (to_find > sorted.back()) {
        return sorted.size() - 1;
    } else {
        unsigned long low = 0, high = sorted.size() - 1;
        while (low < high - 1) {
            unsigned long middle = (low + high) / 2;
            if (sorted[middle] > to_find) {
                high = middle;
            } else {
                low = middle;
            }
        }
        return low;
    }
}

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

void find_depth_one(const std::vector<double>& source, double to_find) {
    double highscore = abs(to_find), best_fit;
    for (double x : source) {
        if (abs(to_find - x) < highscore) {
            highscore = abs(to_find - x);
            best_fit = x;
        }
    }
    std::cout << best_fit << " ";
}

/**
 * TODO add square root resolution
 * @param root
 * @param depth
 * @param sources
 * @param to_find
 */
template<bool ROOT>
void findAndPrint(size_t depth, std::vector<std::vector<double>> &sources, double to_find) {
    if (depth == 1) {
        find_depth_one(sources[1], to_find);
        return;
    }
    SimpleClock clock2;
    if constexpr (ROOT) {
        clock1.start();
    }
    double highscore = abs(to_find) + 10;
    double operand1;
    std::string best_op;
    size_t larger_depth = depth - 1, depth1;
    double operand2;
    if (larger_depth >= sources.size()) {
        std::cout << "Jumping from search depth " << larger_depth << " down to " << sources.size() - 1 << std::endl;
        larger_depth = sources.size() - 1;
    }
    while (larger_depth * 2 >= depth) {
        const std::vector<double>& large = sources[larger_depth];
        const std::vector<double>& small = sources[depth - larger_depth];
        if constexpr (ROOT) {
            clock2.start();
        }
        for (size_t i = 0, j = small.size() - 1; i < large.size() && j <= small.size();) {
            double x = large[i] + small[j];
            if (abs(to_find - x) < highscore) {
                highscore = abs(to_find - x);
                best_op = '+';
                operand1 = large[i];
                operand2 = small[j];
                depth1 = larger_depth;
            }
            if (x > to_find) {
                j--;
            } else {
                i++;
            }
        }
        if constexpr (ROOT) {
            std::cout << "Adding " << clock2.end() << std::endl;
            clock2.start();
        }
        for (size_t i = 0, j = 0; i < large.size() && j < small.size();) {
            double x = large[i] - small[j];
            if (abs(to_find - x) < highscore) {
                highscore = abs(to_find - x);
                best_op = '-';
                operand1 = large[i];
                operand2 = small[j];
                depth1 = larger_depth;
            }
            if (x > to_find) {
                j++;
            } else {
                i++;
            }
        }
        if constexpr (ROOT) {
            std::cout << "Sub1 " << clock2.end() << std::endl;
            clock2.start();
        }
        for (size_t i = 0, j = 0; i < large.size() && j < small.size();) {
            double x = small[j] - large[i];
            if (abs(to_find - x) < highscore) {
                highscore = abs(to_find - x);
                best_op = '-';
                operand1 = small[j];
                operand2 = large[i];
                depth1 = depth - larger_depth;
            }
            if (x > to_find) {
                i++;
            } else {
                j++;
            }
        }
        if constexpr (ROOT) {
            std::cout << "Sub2 " << clock2.end() << std::endl;
            clock2.start();
        }

        if (to_find >= 0) { // we assume both factors are positive, negative * negative should rarely be optimal
            size_t largeZero = approxBinSearch(large, 0);
            size_t smallZero = approxBinSearch(small, 0);

            for (size_t i = largeZero, j = small.size() - 1; i < large.size() && j >= smallZero && j <= small.size();) {
                double x = large[i] * small[j];
                if (abs(to_find - x) < highscore) {
                    highscore = abs(to_find - x);
                    best_op = '*';
                    operand1 = large[i];
                    operand2 = small[j];
                    depth1 = larger_depth;
                }
                if (x > to_find) {
                    j--;
                } else {
                    i++;
                }
            }
            if constexpr (ROOT) {
                std::cout << "Mul " << clock2.end() << std::endl;
                clock2.start();
            }

            for (size_t i = largeZero, j = smallZero; i < large.size() && j < small.size();) {
                double x = large[i] / small[j];
                if (abs(to_find - x) < highscore) {
                    highscore = abs(to_find - x);
                    best_op = '/';
                    operand1 = large[i];
                    operand2 = small[j];
                    depth1 = larger_depth;
                }
                if (x > to_find) {
                    j++;
                } else {
                    i++;
                }
            }
            if constexpr (ROOT) {
                std::cout << "Div1 " << clock2.end() << std::endl;
                clock2.start();
            }
            for (size_t i = largeZero, j = smallZero; i < large.size() && j < small.size();) {
                double x = small[j] / large[i];
                if (abs(to_find - x) < highscore) {
                    highscore = abs(to_find - x);
                    best_op = '/';
                    operand1 = small[j];
                    operand2 = large[i];
                    depth1 = depth - larger_depth;
                }
                if (x > to_find) {
                    i++;
                } else {
                    j++;
                }
            }
            if constexpr (ROOT) {
                std::cout << "Div2 " << clock2.end() << std::endl;
                clock2.start();
            }
            for (int i = 0; i < large.size(); i++) {
                for (int j = 0; j < small.size(); j++) {
                    double x = pow(large[i], small[j]);
                    if (abs(to_find - x) < highscore) {
                        highscore = abs(to_find - x);
                        best_op = "pow";
                        operand1 = large[i];
                        operand2 = small[j];
                        depth1 = larger_depth;
                    }
                    if (x > to_find) {

                    }
                }
            }
            if constexpr (ROOT) {
                std::cout << "Pow1 " << clock2.end() << std::endl;
                clock2.start();
            }
        } else {
            for (int i = 0; i < large.size(); i++) {
                for (int j = 0; j < small.size(); j++) {
                    double x = large[i] * small[j];
                    if (abs(to_find - x) < highscore) {
                        highscore = abs(to_find - x);
                        best_op = '*';
                        operand1 = large[i];
                        operand2 = small[j];
                        depth1 = larger_depth;
                    }
                }
            }
            if constexpr (ROOT) {
                std::cout << "Slow Mul " << clock2.end() << std::endl;
                clock2.start();
            }
            for (int i = 0; i < large.size(); i++) {
                for (int j = 0; j < small.size(); j++) {
                    double x = large[i] / small[j];
                    if (abs(to_find - x) < highscore) {
                        highscore = abs(to_find - x);
                        best_op = '/';
                        operand1 = large[i];
                        operand2 = small[j];
                        depth1 = larger_depth;
                    }
                }
            }

            if constexpr (ROOT) {
                std::cout << "Slow Div1 " << clock2.end() << std::endl;
                clock2.start();
            }
            for (int i = 0; i < large.size(); i++) {
                for (int j = 0; j < small.size(); j++) {
                    double x = small[j] / large[i];
                    if (abs(to_find - x) < highscore) {
                        highscore = abs(to_find - x);
                        best_op = '/';
                        operand1 = small[j];
                        operand2 = large[i];
                        depth1 = depth - larger_depth;
                    }
                }
            }
            if constexpr (ROOT) {
                std::cout << "Slow Div2 " << clock2.end() << std::endl;
                clock2.start();
            }
            for (int i = 0; i < large.size(); i++) {
                for (int j = 0; j < small.size(); j++) {
                    double x = pow(large[i], small[j]);
                    if (abs(to_find - x) < highscore) {
                        highscore = abs(to_find - x);
                        best_op = "pow";
                        operand1 = large[i];
                        operand2 = small[j];
                        depth1 = larger_depth;
                    }
                }
            }
            if constexpr (ROOT) {
                std::cout << "Slow Pow1 " << clock2.end() << std::endl;
                clock2.start();
            }
        }

        for (int i = 0; i < large.size(); i++) {
            for (int j = 0; j < small.size(); j++) {
                double x = pow(small[j], large[i]);
                if (abs(to_find - x) < highscore) {
                    highscore = abs(to_find - x);
                    best_op = "pow";
                    operand1 = small[j];
                    operand2 = large[i];
                    depth1 = depth - larger_depth;
                }
            }
        }
        if constexpr (ROOT) {
            std::cout << "Slow Pow2 " << clock2.end() << std::endl;
            clock2.start();
        }
        for (int i = 0; i < large.size(); i++) {
            for (int j = 0; j < small.size(); j++) {
                double x = log(large[i]) / log(small[j]);
                if (abs(to_find - x) < highscore) {
                    highscore = abs(to_find - x);
                    best_op = "log";
                    operand1 = large[i];
                    operand2 = small[j];
                    depth1 = larger_depth;
                }
            }
        }
        if constexpr (ROOT) {
            std::cout << "Slow Log1 " << clock2.end() << std::endl;
            clock2.start();
        }
        for (int i = 0; i < large.size(); i++) {
            for (int j = 0; j < small.size(); j++) {
                double x = log(small[j]) / log(large[i]);
                if (abs(to_find - x) < highscore) {
                    highscore = abs(to_find - x);
                    best_op = "log";
                    operand1 = small[j];
                    operand2 = large[i];
                    depth1 = depth - larger_depth;
                }
            }
        }
        if constexpr (ROOT) {
            std::cout << "Slow Log2 " << clock2.end() << std::endl;
            clock2.start();
        }
        larger_depth--;
    }
    if constexpr (ROOT) {
        std::cout << clock1.end() << " seconds, depth: " << depth << ", highscore: " << highscore << "; x =  ";
    }
    std::cout << best_op << " ";
    findAndPrint<false>(depth1, sources, operand1);
    findAndPrint<false>(depth - depth1, sources, operand2);
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
            size_t index = approxBinSearch(values, x); {
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

    for (size_t depth = 2; depth <= 6; depth++) {
        findAndPrint<true>(depth, values_per_depth, to_find);
        std::cout << std::endl;

        generate_values(values_per_depth, depth);
        count_matching_negative_numbers(values_per_depth[depth]);
    }

    findAndPrint<true>(7, values_per_depth, to_find);
    std::cout << std::endl;
    findAndPrint<true>(8, values_per_depth, to_find);
    std::cout << std::endl;
    return 0;
}