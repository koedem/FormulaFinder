#pragma once

#include <iostream>
#include <cmath>
#include <vector>
#include <cassert>
#include <algorithm>
#include <string>
#include <utility>
#include "SimpleClock.h"
#include "Utils.h"
#include "AddMerger.h"
#include "Sub1Merger.h"
#include "Sub2Merger.h"

class MergeFinder {

    friend struct MergeFinderTest; // grants the validation harness access to the private merge paths.

private:
    struct FormulaData {
        double operand1, operand2;
        double absolute_difference;
        Utils::Op operation;
        size_t depth1; // Remaining depth for operand1
    };

    template<bool ROOT, Utils::Op OP>
    void find_generic_sum(const std::vector<double>& large, const std::vector<double>& small, double to_find,
                       FormulaData& best, const size_t first_depth, SimpleClock& clock) {
        if constexpr (ROOT) {
            clock.start();
        }
        for (size_t i = starting_i<OP>(large.size()), j = starting_j<OP>(small.size()); condition<OP>(i, j, large.size(), small.size());) {
            double x = Utils::apply_operator<OP>(large[i], small[j]);
            if (std::abs(to_find - x) < best.absolute_difference) {
                best.absolute_difference = std::abs(to_find - x);
                best.operation = OP;
                best.operand1 = large[i];
                best.operand2 = small[j];
                best.depth1 = first_depth;
            }
            if (x > to_find) {
                update_big_x<OP>(i, j);
            } else {
                update_small_x<OP>(i, j);
            }
        }
        if constexpr (ROOT) {
            std::cout << op_strings[OP] << " " << clock.end() << std::endl;
        }
    }

    static void update_best(double x, double a, double b, Utils::Op op, size_t first_depth,
                            double to_find, FormulaData& best) {
        if (std::isfinite(x) && std::abs(to_find - x) < best.absolute_difference) {
            best.absolute_difference = std::abs(to_find - x);
            best.operation = op;
            best.operand1 = a;
            best.operand2 = b;
            best.depth1 = first_depth;
        }
    }

    /**
     * Two-pointer "closest match" sweep over a rectangular sub-block of the large x small result matrix on which the
     * operation OP is strictly monotone in each operand with a fixed sign. SA / SB are those signs: SA is +1 if
     * increasing the large operand increases OP, -1 otherwise; SB likewise for the small operand. Given that, the
     * sub-matrix is sorted along both axes, so the global closest value is found in O((iHi - iLo) + (jHi - jLo)).
     * The large pointer i always travels forward; the small pointer j travels down when the signs match and up
     * otherwise, providing the opposing control needed to home in on to_find.
     */
    template<Utils::Op OP, int SA, int SB>
    static void merge_region(const std::vector<double>& large, const std::vector<double>& small,
                             size_t iLo, size_t iHi, size_t jLo, size_t jHi,
                             double to_find, FormulaData& best, const size_t first_depth) {
        if (iLo >= iHi || jLo >= jHi) {
            return;
        }
        constexpr int jDir = (SA == SB) ? -1 : 1; // j descends when both signs agree, ascends otherwise.
        size_t i = iLo;
        size_t j = (jDir < 0) ? jHi - 1 : jLo;
        while (true) {
            double x = Utils::apply_operator<OP>(large[i], small[j]);
            update_best(x, large[i], small[j], OP, first_depth, to_find, best);
            // SA > 0: advancing i grows x, so we advance i to grow and step j to shrink (and vice versa for SA < 0).
            bool advance_i = (SA > 0) ? (x <= to_find) : (x > to_find);
            if (advance_i) {
                if (++i >= iHi) {
                    return;
                }
            } else if constexpr (jDir < 0) {
                if (j == jLo) {
                    return;
                }
                j--;
            } else {
                if (++j >= jHi) {
                    return;
                }
            }
        }
    }

    // Split points of a sorted vector at zero: [0, neg) are negative, [neg, pos) are zero, [pos, size) are positive.
    struct SignSplit { size_t neg, pos; };
    static SignSplit sign_split(const std::vector<double>& v) {
        return { static_cast<size_t>(std::lower_bound(v.begin(), v.end(), 0.0) - v.begin()),
                 static_cast<size_t>(std::upper_bound(v.begin(), v.end(), 0.0) - v.begin()) };
    }

    template<bool ROOT>
    void merge_mul(const std::vector<double>& large, const std::vector<double>& small, double to_find,
                   FormulaData& best, const size_t first_depth, SimpleClock& clock) {
        if constexpr (ROOT) { clock.start(); }
        const size_t N = large.size(), M = small.size();
        const auto l = sign_split(large), s = sign_split(small);
        merge_region<Utils::MUL, +1, +1>(large, small, l.pos, N, s.pos, M, to_find, best, first_depth); // a>0,b>0
        merge_region<Utils::MUL, +1, -1>(large, small, 0, l.neg, s.pos, M, to_find, best, first_depth); // a<0,b>0
        merge_region<Utils::MUL, -1, +1>(large, small, l.pos, N, 0, s.neg, to_find, best, first_depth); // a>0,b<0
        merge_region<Utils::MUL, -1, -1>(large, small, 0, l.neg, 0, s.neg, to_find, best, first_depth); // a<0,b<0
        if (l.neg < l.pos) { update_best(0.0, large[l.neg], small.front(), Utils::MUL, first_depth, to_find, best); }
        if (s.neg < s.pos) { update_best(0.0, large.front(), small[s.neg], Utils::MUL, first_depth, to_find, best); }
        if constexpr (ROOT) { std::cout << "Mul " << clock.end() << std::endl; }
    }

    template<bool ROOT>
    void merge_div1(const std::vector<double>& large, const std::vector<double>& small, double to_find,
                    FormulaData& best, const size_t first_depth, SimpleClock& clock) {
        if constexpr (ROOT) { clock.start(); }
        const size_t N = large.size(), M = small.size();
        const auto l = sign_split(large), s = sign_split(small); // a / b, the b = 0 column is undefined and skipped.
        merge_region<Utils::DIV1, +1, -1>(large, small, l.pos, N, s.pos, M, to_find, best, first_depth); // a>0,b>0
        merge_region<Utils::DIV1, +1, +1>(large, small, 0, l.neg, s.pos, M, to_find, best, first_depth); // a<0,b>0
        merge_region<Utils::DIV1, -1, -1>(large, small, l.pos, N, 0, s.neg, to_find, best, first_depth); // a>0,b<0
        merge_region<Utils::DIV1, -1, +1>(large, small, 0, l.neg, 0, s.neg, to_find, best, first_depth); // a<0,b<0
        if (l.neg < l.pos) { // a = 0 over any non-zero b yields 0.
            if (s.pos < M) { update_best(0.0, large[l.neg], small[s.pos], Utils::DIV1, first_depth, to_find, best); }
            else if (s.neg > 0) { update_best(0.0, large[l.neg], small[s.neg - 1], Utils::DIV1, first_depth, to_find, best); }
        }
        if constexpr (ROOT) { std::cout << "Div1 " << clock.end() << std::endl; }
    }

    template<bool ROOT>
    void merge_div2(const std::vector<double>& large, const std::vector<double>& small, double to_find,
                    FormulaData& best, const size_t first_depth, SimpleClock& clock) {
        if constexpr (ROOT) { clock.start(); }
        const size_t N = large.size(), M = small.size();
        const auto l = sign_split(large), s = sign_split(small); // b / a, the a = 0 column is undefined and skipped.
        merge_region<Utils::DIV2, -1, +1>(large, small, l.pos, N, s.pos, M, to_find, best, first_depth); // a>0,b>0
        merge_region<Utils::DIV2, -1, -1>(large, small, 0, l.neg, s.pos, M, to_find, best, first_depth); // a<0,b>0
        merge_region<Utils::DIV2, +1, +1>(large, small, l.pos, N, 0, s.neg, to_find, best, first_depth); // a>0,b<0
        merge_region<Utils::DIV2, +1, -1>(large, small, 0, l.neg, 0, s.neg, to_find, best, first_depth); // a<0,b<0
        if (s.neg < s.pos) { // b = 0 over any non-zero a yields 0.
            if (l.pos < N) { update_best(0.0, large[l.pos], small[s.neg], Utils::DIV2, first_depth, to_find, best); }
            else if (l.neg > 0) { update_best(0.0, large[l.neg - 1], small[s.neg], Utils::DIV2, first_depth, to_find, best); }
        }
        if constexpr (ROOT) { std::cout << "Div2 " << clock.end() << std::endl; }
    }

    template<bool ROOT, Utils::Op OP>
    void brute_force_search(const std::vector<double>& first, const std::vector<double>& second, double to_find,
                            FormulaData& best, const size_t first_depth, SimpleClock& clock2) {
        for (double a : first) {
            for (double b : second) {
                double x = Utils::apply_operator<OP>(a, b);
                if (std::abs(to_find - x) < best.absolute_difference) {
                    best.absolute_difference = std::abs(to_find - x);
                    best.operation = OP;
                    best.operand1 = a;
                    best.operand2 = b;
                    best.depth1 = first_depth;
                }
            }
        }
        if constexpr (ROOT) {
            std::cout << "Slow " << op_strings[OP] << " " << clock2.end() << std::endl;
            clock2.start();
        }
    }

    // Reconstruction reaches a depth-1 operand whose value is exactly one of the atoms; print that atom's symbol.
    void exact_depth_one_match(double to_find) const {
        for (const auto& [value, label] : atoms) {
            if (value == to_find) {
                std::cout << label << " ";
                return;
            }
        }
        assert(false);
    }

    // Top-level depth-1 search: no exact match guaranteed, so print the symbol of the closest atom.
    void find_depth_one(double to_find) const {
        const std::string* best_label = nullptr;
        double high_score = std::abs(to_find);
        for (const auto& [value, label] : atoms) {
            if (best_label == nullptr || std::abs(to_find - value) < high_score) {
                high_score = std::abs(to_find - value);
                best_label = &label;
            }
        }
        std::cout << *best_label << " ";
    }

public:

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
            if constexpr (ROOT) { // If this is a depth 1 search we might not get an exact match.
                find_depth_one(to_find);
            } else {
                exact_depth_one_match(to_find);
            }
            return;
        }
        SimpleClock clock2;
        if constexpr (ROOT) {
            clock1.start();
        }
        FormulaData best{};
        best.absolute_difference = std::abs(to_find) + 10;
        size_t larger_depth = depth - 1;
        if (larger_depth >= sources.size()) {
            std::cout << "Jumping from search depth " << larger_depth << " down to " << sources.size() - 1 << std::endl;
            larger_depth = sources.size() - 1;
        }
        while (larger_depth * 2 >= depth) {
            const std::vector<double>& large = sources[larger_depth];
            const std::vector<double>& small = sources[depth - larger_depth];
            find_generic_sum<ROOT, Utils::ADD>(large, small, to_find, best, larger_depth, clock2);
            find_generic_sum<ROOT, Utils::SUB1>(large, small, to_find, best, larger_depth, clock2);
            find_generic_sum<ROOT, Utils::SUB2>(large, small, to_find, best, larger_depth, clock2);

            merge_mul<ROOT>(large, small, to_find, best, larger_depth, clock2);
            merge_div1<ROOT>(large, small, to_find, best, larger_depth, clock2);
            merge_div2<ROOT>(large, small, to_find, best, larger_depth, clock2);

            if constexpr (ROOT) { clock2.start(); }
            brute_force_search<ROOT, Utils::POW1>(large, small, to_find, best, larger_depth, clock2);
            brute_force_search<ROOT, Utils::POW2>(large, small, to_find, best, larger_depth, clock2);
            brute_force_search<ROOT, Utils::LOG1>(large, small, to_find, best, larger_depth, clock2);
            brute_force_search<ROOT, Utils::LOG2>(large, small, to_find, best, larger_depth, clock2);

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
                findAndPrint<false>(depth - best.depth1, sources, best.operand2);
                findAndPrint<false>(best.depth1, sources, best.operand1);
                break;
            default:
                findAndPrint<false>(best.depth1, sources, best.operand1);
                findAndPrint<false>(depth - best.depth1, sources, best.operand2);
                break;
        }
    }

    MergeFinder(SimpleClock& clock1, const std::vector<std::pair<double, std::string>>& atoms)
        : clock1(clock1), atoms(atoms) {
    }

private:
    SimpleClock& clock1;
    const std::vector<std::pair<double, std::string>>& atoms; // depth-1 {value, symbol} pairs, for printing leaves.

    static constexpr std::string_view op_strings[] = {[Utils::ADD] = "+", [Utils::SUB1] = "-", [Utils::SUB2] = "-",
                                [Utils::MUL] = "*", [Utils::DIV1] = "/", [Utils::DIV2] = "/", [Utils::POW1] = "pow1",
                                [Utils::POW2] = "pow2", [Utils::LOG1] = "log1", [Utils::LOG2] = "log2" };
};
