#pragma once

#include <iostream>
#include <cmath>
#include <vector>
#include <algorithm>
#include <string_view>
#include "SimpleClock.h"
#include "Operators.h"
#include "Real.h"
#include "Log.h"

/**
 * The numeric search engine: given the sorted value tiers and a target, it finds the single closest formula of a
 * given depth and returns it as a FormulaData (operands, operator, and how the depth splits between them). It deals
 * only in numbers -- no atoms, constants, tree building, or printing -- which all live in FormulaFinder.
 */
class MergeFinder {

    friend struct MergeFinderTest; // grants the validation harness access to the private merge paths.

public:
    // The closest match the search found: the two operand values, their absolute distance to the target, the
    // operator relating them, and how much of the total depth went to operand1 (operand2 gets the rest).
    struct FormulaData {
        Real operand1, operand2;
        Real absolute_difference;
        Operators::Op operation;
        size_t depth1; // Remaining depth for operand1
    };

    /**
     * Searches for the closest formula of the given depth to to_find over the value tiers. Considers every binary
     * operator across the sorted tiers plus the unary sqrt of the depth-(d-1) tier. ROOT only affects whether the
     * per-operation debug timing is emitted (reconstruction's re-searches run with ROOT = false and stay silent).
     * @param depth target expression depth (>= 2).
     * @param sources value tiers, sources[k] holding every value reachable at depth k.
     * @param to_find the value to approximate.
     */
    template<bool ROOT>
    static FormulaData search(size_t depth, const std::vector<std::vector<Real>>& sources, Real to_find) {
        FormulaData best{};
        best.absolute_difference = std::abs(to_find) + 10;
        size_t larger_depth = depth - 1;
        if (larger_depth >= sources.size()) {
            LOG_AT(LogLevel::INFO) << "Jumping from search depth " << larger_depth << " down to " << sources.size() - 1 << std::endl;
            larger_depth = sources.size() - 1;
        }
        while (larger_depth * 2 >= depth) {
            const std::vector<Real>& large = sources[larger_depth];
            const std::vector<Real>& small = sources[depth - larger_depth];
            merge_full<ROOT, Operators::ADD, +1, +1>(large, small, to_find, best, larger_depth);   // a+b
            merge_full<ROOT, Operators::SUB1, +1, -1>(large, small, to_find, best, larger_depth);  // a-b
            merge_full<ROOT, Operators::SUB2, -1, +1>(large, small, to_find, best, larger_depth);  // b-a

            merge_mul<ROOT>(large, small, to_find, best, larger_depth);
            merge_div1<ROOT>(large, small, to_find, best, larger_depth);
            merge_div2<ROOT>(large, small, to_find, best, larger_depth);
            merge_pow1<ROOT>(large, small, to_find, best, larger_depth);
            merge_pow2<ROOT>(large, small, to_find, best, larger_depth);
            merge_log1<ROOT>(large, small, to_find, best, larger_depth);
            merge_log2<ROOT>(large, small, to_find, best, larger_depth);

            larger_depth--;
        }
        // sqrt is unary and costs exactly one depth, so it draws on the depth-(d-1) tier -- but only when that tier
        // exists. For the jumped-to high depths (depth - 1 >= sources.size()) it was never generated, so skip it,
        // matching the fact that no sqrt term was injected into those depths either.
        if (depth - 1 < sources.size()) {
            merge_sqrt<ROOT>(sources[depth - 1], to_find, best, depth - 1);
        }
        return best;
    }

private:
    // Times one merge operation and prints "<name> <seconds>" on scope exit. Active only for the top-level (ROOT)
    // search and only at DEBUG verbosity, so reconstruction's re-searches (root = false) stay silent and a clock is
    // never even started when the breakdown would not be shown.
    struct OpTimer {
        std::string_view name;
        SimpleClock clock;
        bool active;
        OpTimer(std::string_view name, bool root) : name(name), active(root && Log::enabled(LogLevel::DEBUG)) {
            if (active) { clock.start(); }
        }
        ~OpTimer() {
            if (active) { std::cout << name << " " << clock.end() << '\n'; }
        }
    };

    // ADD / SUB1 / SUB2 are strictly monotone in each operand with a fixed sign over the *entire* large x small
    // rectangle (no sign-dependent sub-regions), so each reduces to a single full-block merge_region sweep.
    template<bool ROOT, Operators::Op OP, int SA, int SB>
    static void merge_full(const std::vector<Real>& large, const std::vector<Real>& small, Real to_find,
                           FormulaData& best, const size_t first_depth) {
        OpTimer t(Operators::symbol(OP), ROOT);
        merge_region<OP, SA, SB>(large, small, 0, large.size(), 0, small.size(), to_find, best, first_depth);
    }

    static void update_best(Real x, Real a, Real b, Operators::Op op, size_t first_depth,
                            Real to_find, FormulaData& best) {
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
    template<Operators::Op OP, int SA, int SB>
    static void merge_region(const std::vector<Real>& large, const std::vector<Real>& small,
                             size_t iLo, size_t iHi, size_t jLo, size_t jHi,
                             Real to_find, FormulaData& best, const size_t first_depth) {
        if (iLo >= iHi || jLo >= jHi) {
            return;
        }
        constexpr int jDir = (SA == SB) ? -1 : 1; // j descends when both signs agree, ascends otherwise.
        size_t i = iLo;
        size_t j = (jDir < 0) ? jHi - 1 : jLo;
        while (true) {
            Real x = Operators::apply_operator<OP>(large[i], small[j]);
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
    static SignSplit sign_split(const std::vector<Real>& v) {
        return { static_cast<size_t>(std::lower_bound(v.begin(), v.end(), 0.0) - v.begin()),
                 static_cast<size_t>(std::upper_bound(v.begin(), v.end(), 0.0) - v.begin()) };
    }

    // Split points of a sorted vector for pow/log: [pos, oneLo) lies in (0, 1), [oneLo, oneHi) equals 1,
    // [oneHi, size) is > 1. Values <= 0 (indices [0, pos)) are outside the real/monotone domain and ignored.
    struct PosSplit { size_t pos, oneLo, oneHi; };
    static PosSplit pos_split(const std::vector<Real>& v) {
        return { static_cast<size_t>(std::upper_bound(v.begin(), v.end(), 0.0) - v.begin()),
                 static_cast<size_t>(std::lower_bound(v.begin(), v.end(), 1.0) - v.begin()),
                 static_cast<size_t>(std::upper_bound(v.begin(), v.end(), 1.0) - v.begin()) };
    }

    template<bool ROOT>
    static void merge_mul(const std::vector<Real>& large, const std::vector<Real>& small, Real to_find,
                          FormulaData& best, const size_t first_depth) {
        OpTimer t("Mul", ROOT);
        const size_t N = large.size(), M = small.size();
        const auto l = sign_split(large), s = sign_split(small);
        merge_region<Operators::MUL, +1, +1>(large, small, l.pos, N, s.pos, M, to_find, best, first_depth); // a>0,b>0
        merge_region<Operators::MUL, +1, -1>(large, small, 0, l.neg, s.pos, M, to_find, best, first_depth); // a<0,b>0
        merge_region<Operators::MUL, -1, +1>(large, small, l.pos, N, 0, s.neg, to_find, best, first_depth); // a>0,b<0
        merge_region<Operators::MUL, -1, -1>(large, small, 0, l.neg, 0, s.neg, to_find, best, first_depth); // a<0,b<0
        if (l.neg < l.pos) { update_best(0.0, large[l.neg], small.front(), Operators::MUL, first_depth, to_find, best); }
        if (s.neg < s.pos) { update_best(0.0, large.front(), small[s.neg], Operators::MUL, first_depth, to_find, best); }
    }

    template<bool ROOT>
    static void merge_div1(const std::vector<Real>& large, const std::vector<Real>& small, Real to_find,
                           FormulaData& best, const size_t first_depth) {
        OpTimer t("Div1", ROOT);
        const size_t N = large.size(), M = small.size();
        const auto l = sign_split(large), s = sign_split(small); // a / b, the b = 0 column is undefined and skipped.
        merge_region<Operators::DIV1, +1, -1>(large, small, l.pos, N, s.pos, M, to_find, best, first_depth); // a>0,b>0
        merge_region<Operators::DIV1, +1, +1>(large, small, 0, l.neg, s.pos, M, to_find, best, first_depth); // a<0,b>0
        merge_region<Operators::DIV1, -1, -1>(large, small, l.pos, N, 0, s.neg, to_find, best, first_depth); // a>0,b<0
        merge_region<Operators::DIV1, -1, +1>(large, small, 0, l.neg, 0, s.neg, to_find, best, first_depth); // a<0,b<0
        if (l.neg < l.pos) { // a = 0 over any non-zero b yields 0.
            if (s.pos < M) { update_best(0.0, large[l.neg], small[s.pos], Operators::DIV1, first_depth, to_find, best); }
            else if (s.neg > 0) { update_best(0.0, large[l.neg], small[s.neg - 1], Operators::DIV1, first_depth, to_find, best); }
        }
    }

    template<bool ROOT>
    static void merge_div2(const std::vector<Real>& large, const std::vector<Real>& small, Real to_find,
                           FormulaData& best, const size_t first_depth) {
        OpTimer t("Div2", ROOT);
        const size_t N = large.size(), M = small.size();
        const auto l = sign_split(large), s = sign_split(small); // b / a, the a = 0 column is undefined and skipped.
        merge_region<Operators::DIV2, -1, +1>(large, small, l.pos, N, s.pos, M, to_find, best, first_depth); // a>0,b>0
        merge_region<Operators::DIV2, -1, -1>(large, small, 0, l.neg, s.pos, M, to_find, best, first_depth); // a<0,b>0
        merge_region<Operators::DIV2, +1, +1>(large, small, l.pos, N, 0, s.neg, to_find, best, first_depth); // a>0,b<0
        merge_region<Operators::DIV2, +1, -1>(large, small, 0, l.neg, 0, s.neg, to_find, best, first_depth); // a<0,b<0
        if (s.neg < s.pos) { // b = 0 over any non-zero a yields 0.
            if (l.pos < N) { update_best(0.0, large[l.pos], small[s.neg], Operators::DIV2, first_depth, to_find, best); }
            else if (l.neg > 0) { update_best(0.0, large[l.neg - 1], small[s.neg], Operators::DIV2, first_depth, to_find, best); }
        }
    }

    // pow(a, b): base a = large (split at 1, must be > 0), exponent b = small (split at 0).
    // d/da = b*a^(b-1) so SA = sign(b); d/db = a^b*ln(a) so SB = sign(a - 1).
    template<bool ROOT>
    static void merge_pow1(const std::vector<Real>& large, const std::vector<Real>& small, Real to_find,
                           FormulaData& best, const size_t first_depth) {
        OpTimer t("pow1", ROOT);
        const size_t N = large.size(), M = small.size();
        const auto a = pos_split(large);
        const auto b = sign_split(small);
        merge_region<Operators::POW1, +1, +1>(large, small, a.oneHi, N, b.pos, M, to_find, best, first_depth); // a>1,b>0
        merge_region<Operators::POW1, +1, -1>(large, small, a.pos, a.oneLo, b.pos, M, to_find, best, first_depth); // 0<a<1,b>0
        merge_region<Operators::POW1, -1, +1>(large, small, a.oneHi, N, 0, b.neg, to_find, best, first_depth); // a>1,b<0
        merge_region<Operators::POW1, -1, -1>(large, small, a.pos, a.oneLo, 0, b.neg, to_find, best, first_depth); // 0<a<1,b<0
        if (a.oneLo < a.oneHi) { update_best(1.0, large[a.oneLo], small.front(), Operators::POW1, first_depth, to_find, best); } // 1^b = 1
        if (b.neg < b.pos && a.pos < N) { update_best(1.0, large[a.pos], small[b.neg], Operators::POW1, first_depth, to_find, best); } // a^0 = 1
    }

    // pow(b, a): base b = small (split at 1, must be > 0), exponent a = large (split at 0).
    // d/da = b^a*ln(b) so SA = sign(b - 1); d/db = a*b^(a-1) so SB = sign(a).
    template<bool ROOT>
    static void merge_pow2(const std::vector<Real>& large, const std::vector<Real>& small, Real to_find,
                           FormulaData& best, const size_t first_depth) {
        OpTimer t("pow2", ROOT);
        const size_t N = large.size(), M = small.size();
        const auto a = sign_split(large);
        const auto b = pos_split(small);
        merge_region<Operators::POW2, +1, +1>(large, small, a.pos, N, b.oneHi, M, to_find, best, first_depth); // b>1,a>0
        merge_region<Operators::POW2, +1, -1>(large, small, 0, a.neg, b.oneHi, M, to_find, best, first_depth); // b>1,a<0
        merge_region<Operators::POW2, -1, +1>(large, small, a.pos, N, b.pos, b.oneLo, to_find, best, first_depth); // 0<b<1,a>0
        merge_region<Operators::POW2, -1, -1>(large, small, 0, a.neg, b.pos, b.oneLo, to_find, best, first_depth); // 0<b<1,a<0
        if (b.oneLo < b.oneHi) { update_best(1.0, large.front(), small[b.oneLo], Operators::POW2, first_depth, to_find, best); } // 1^a = 1
        if (a.neg < a.pos && b.pos < M) { update_best(1.0, large[a.neg], small[b.pos], Operators::POW2, first_depth, to_find, best); } // b^0 = 1
    }

    // log(a)/log(b) = log_b(a): a = large, b = small, both split at 1 and required > 0 (b != 1).
    // SA = sign(b - 1); SB = -sign(a - 1).
    template<bool ROOT>
    static void merge_log1(const std::vector<Real>& large, const std::vector<Real>& small, Real to_find,
                           FormulaData& best, const size_t first_depth) {
        OpTimer t("log1", ROOT);
        const size_t N = large.size(), M = small.size();
        const auto a = pos_split(large), b = pos_split(small);
        merge_region<Operators::LOG1, +1, -1>(large, small, a.oneHi, N, b.oneHi, M, to_find, best, first_depth); // a>1,b>1
        merge_region<Operators::LOG1, +1, +1>(large, small, a.pos, a.oneLo, b.oneHi, M, to_find, best, first_depth); // 0<a<1,b>1
        merge_region<Operators::LOG1, -1, -1>(large, small, a.oneHi, N, b.pos, b.oneLo, to_find, best, first_depth); // a>1,0<b<1
        merge_region<Operators::LOG1, -1, +1>(large, small, a.pos, a.oneLo, b.pos, b.oneLo, to_find, best, first_depth); // 0<a<1,0<b<1
        if (a.oneLo < a.oneHi) { // log_b(1) = 0 for any valid base b != 1.
            if (b.oneHi < M) { update_best(0.0, large[a.oneLo], small[b.oneHi], Operators::LOG1, first_depth, to_find, best); }
            else if (b.pos < b.oneLo) { update_best(0.0, large[a.oneLo], small[b.pos], Operators::LOG1, first_depth, to_find, best); }
        }
    }

    // log(b)/log(a) = log_a(b): a = large, b = small, both split at 1 and required > 0 (a != 1).
    // SA = -sign(b - 1); SB = sign(a - 1).
    template<bool ROOT>
    static void merge_log2(const std::vector<Real>& large, const std::vector<Real>& small, Real to_find,
                           FormulaData& best, const size_t first_depth) {
        OpTimer t("log2", ROOT);
        const size_t N = large.size(), M = small.size();
        const auto a = pos_split(large), b = pos_split(small);
        merge_region<Operators::LOG2, -1, +1>(large, small, a.oneHi, N, b.oneHi, M, to_find, best, first_depth); // a>1,b>1
        merge_region<Operators::LOG2, +1, +1>(large, small, a.oneHi, N, b.pos, b.oneLo, to_find, best, first_depth); // a>1,0<b<1
        merge_region<Operators::LOG2, -1, -1>(large, small, a.pos, a.oneLo, b.oneHi, M, to_find, best, first_depth); // 0<a<1,b>1
        merge_region<Operators::LOG2, +1, -1>(large, small, a.pos, a.oneLo, b.pos, b.oneLo, to_find, best, first_depth); // 0<a<1,0<b<1
        if (b.oneLo < b.oneHi) { // log_a(1) = 0 for any valid base a != 1.
            if (a.oneHi < N) { update_best(0.0, large[a.oneHi], small[b.oneLo], Operators::LOG2, first_depth, to_find, best); }
            else if (a.pos < a.oneLo) { update_best(0.0, large[a.pos], small[b.oneLo], Operators::LOG2, first_depth, to_find, best); }
        }
    }

    // Unary sqrt over a single sorted source: sqrt costs one depth, so this considers sqrt(v) for v in the
    // depth-(d-1) values. sqrt is monotone increasing on v >= 0, so the closest sqrt(v) to to_find sits at the v
    // closest to to_find^2 -- one binary search plus its two neighbours, rather than a full sweep.
    template<bool ROOT>
    static void merge_sqrt(const std::vector<Real>& source, Real to_find, FormulaData& best, const size_t first_depth) {
        OpTimer t("sqrt", ROOT);
        const size_t pos = static_cast<size_t>(std::lower_bound(source.begin(), source.end(), 0.0) - source.begin());
        if (pos >= source.size()) { return; } // no non-negative radicands
        auto consider = [&](size_t idx) {
            update_best(std::sqrt(source[idx]), source[idx], 0.0, Operators::SQRT, first_depth, to_find, best);
        };
        if (to_find <= 0.0) { // every sqrt(v) >= 0 >= to_find, so the smallest radicand is closest
            consider(pos);
            return;
        }
        const Real target = to_find * to_find;
        const size_t idx = static_cast<size_t>(std::lower_bound(source.begin() + pos, source.end(), target) - source.begin());
        if (idx < source.size()) { consider(idx); }      // smallest v >= to_find^2
        if (idx > pos)          { consider(idx - 1); }   // largest v < to_find^2
    }

    MergeFinder() = default;
};