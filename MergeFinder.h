#pragma once

#include <iostream>
#include <cmath>
#include <vector>
#include "SimpleClock.h"
#include "Utils.h"

class MergeFinder {

private:
    struct FormulaData {
        double operand1, operand2;
        double absolute_difference;
        Utils::Op operation;
    };

public:
    /*template<bool ROOT>
    void find_best_sum(const std::vector<double>& large, const std::vector<double>& small, double to_find, SimpleClock& clock,
                       double& highscore, double& operand1, double& operand2, std::string& best_op, size_t& depth1, const size_t& larger_depth) {
        if constexpr (ROOT) {
            clock.start();
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
            std::cout << "Adding " << clock.end() << std::endl;
            clock.start();
        }

    }*/

    template<bool ROOT, Utils::Op OP>
    void brute_force_search(const std::vector<double>& first, const std::vector<double>& second, double to_find,
                            FormulaData& best, size_t& depth1, const size_t first_depth, SimpleClock& clock2) {
        for (double a : first) {
            for (double b : second) {
                double x = Utils::apply_operator<OP>(a, b);
                if (std::abs(to_find - x) < best.absolute_difference) {
                    best.absolute_difference = std::abs(to_find - x);
                    best.operation = OP;
                    best.operand1 = a;
                    best.operand2 = b;
                    depth1 = first_depth;
                }
            }
        }

        if constexpr (ROOT) {
            std::cout << "Slow " << op_strings[OP] << " " << clock2.end() << std::endl;
            clock2.start();
        }
    }

    // TODO shouldn't we search for exact matches here almost always?
    static void find_depth_one(const std::vector<double>& source, double to_find) {
        double high_score = std::abs(to_find), best_fit;
        for (double x : source) {
            if (std::abs(to_find - x) < high_score) {
                high_score = std::abs(to_find - x);
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
        FormulaData best{};
        best.absolute_difference = std::abs(to_find) + 10;
        size_t larger_depth = depth - 1, depth1;
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
                if (std::abs(to_find - x) < best.absolute_difference) {
                    best.absolute_difference = std::abs(to_find - x);
                    best.operation = Utils::ADD;
                    best.operand1 = large[i];
                    best.operand2 = small[j];
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
                if (std::abs(to_find - x) < best.absolute_difference) {
                    best.absolute_difference = std::abs(to_find - x);
                    best.operation = Utils::SUB1;
                    best.operand1 = large[i];
                    best.operand2 = small[j];
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
                if (std::abs(to_find - x) < best.absolute_difference) {
                    best.absolute_difference = std::abs(to_find - x);
                    best.operation = Utils::SUB2;
                    best.operand1 = large[i];
                    best.operand2 = small[j];
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
                size_t largeZero = Utils::approxBinSearch(large, 0);
                size_t smallZero = Utils::approxBinSearch(small, 0);

                for (size_t i = largeZero, j = small.size() - 1; i < large.size() && j >= smallZero && j <= small.size();) {
                    double x = large[i] * small[j];
                    if (std::abs(to_find - x) < best.absolute_difference) {
                        best.absolute_difference = std::abs(to_find - x);
                        best.operation = Utils::MUL;
                        best.operand1 = large[i];
                        best.operand2 = small[j];
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
                    if (std::abs(to_find - x) < best.absolute_difference) {
                        best.absolute_difference = std::abs(to_find - x);
                        best.operation = Utils::DIV1;
                        best.operand1 = large[i];
                        best.operand2 = small[j];
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
                    if (std::abs(to_find - x) < best.absolute_difference) {
                        best.absolute_difference = std::abs(to_find - x);
                        best.operation = Utils::DIV2;
                        best.operand1 = large[i];
                        best.operand2 = small[j];
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
                brute_force_search<ROOT, Utils::POW1>(large, small, to_find, best, depth1, larger_depth, clock2);
            } else {
                brute_force_search<ROOT, Utils::MUL>(large, small, to_find, best, depth1, larger_depth, clock2);
                brute_force_search<ROOT, Utils::DIV1>(large, small, to_find, best, depth1, larger_depth, clock2);
                brute_force_search<ROOT, Utils::DIV2>(large, small, to_find, best, depth1, depth - larger_depth, clock2);
                brute_force_search<ROOT, Utils::POW1>(large, small, to_find, best, depth1, larger_depth, clock2);
            }

            brute_force_search<ROOT, Utils::POW2>(large, small, to_find, best, depth1, depth - larger_depth, clock2);
            brute_force_search<ROOT, Utils::LOG1>(large, small, to_find, best, depth1, larger_depth, clock2);
            brute_force_search<ROOT, Utils::LOG2>(large, small, to_find, best, depth1, depth - larger_depth, clock2);

            larger_depth--;
        }
        if constexpr (ROOT) {
            std::cout << clock1.end() << " seconds, depth: " << depth << ", high score: " << best.absolute_difference << "; x =  ";
        }
        std::cout << op_strings[best.operation] << " ";

        switch (best.operation) {
            case Utils::SUB2:
            case Utils::DIV2:
            case Utils::POW2:
            case Utils::LOG2:
                findAndPrint<false>(depth1, sources, best.operand2);
                findAndPrint<false>(depth - depth1, sources, best.operand1);
                break;
            default:
                findAndPrint<false>(depth1, sources, best.operand1);
                findAndPrint<false>(depth - depth1, sources, best.operand2);
                break;
        }
    }

    explicit MergeFinder(SimpleClock& clock1) : clock1(clock1) {
    }

private:
    SimpleClock& clock1;

    static constexpr std::string_view op_strings[] = {[Utils::ADD] = "+", [Utils::SUB1] = "-", [Utils::SUB2] = "-",
                                [Utils::MUL] = "*", [Utils::DIV1] = "/", [Utils::DIV2] = "/", [Utils::POW1] = "pow1",
                                [Utils::POW2] = "pow2", [Utils::LOG1] = "log1", [Utils::LOG2] = "log2" };
};
