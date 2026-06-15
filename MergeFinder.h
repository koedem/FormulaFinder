#pragma once

#include <iostream>
#include <cmath>
#include <vector>
#include <cassert>
#include <algorithm>
#include <string>
#include <string_view>
#include <utility>
#include <memory>
#include "SimpleClock.h"
#include "Utils.h"
#include "Log.h"

class MergeFinder {

    friend struct MergeFinderTest; // grants the validation harness access to the private merge paths.

private:
    struct FormulaData {
        double operand1, operand2;
        double absolute_difference;
        Utils::Op operation;
        size_t depth1; // Remaining depth for operand1
    };

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
    template<bool ROOT, Utils::Op OP, int SA, int SB>
    void merge_full(const std::vector<double>& large, const std::vector<double>& small, double to_find,
                    FormulaData& best, const size_t first_depth) {
        OpTimer t(op_strings[OP], ROOT);
        merge_region<OP, SA, SB>(large, small, 0, large.size(), 0, small.size(), to_find, best, first_depth);
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

    // Split points of a sorted vector for pow/log: [pos, oneLo) lies in (0, 1), [oneLo, oneHi) equals 1,
    // [oneHi, size) is > 1. Values <= 0 (indices [0, pos)) are outside the real/monotone domain and ignored.
    struct PosSplit { size_t pos, oneLo, oneHi; };
    static PosSplit pos_split(const std::vector<double>& v) {
        return { static_cast<size_t>(std::upper_bound(v.begin(), v.end(), 0.0) - v.begin()),
                 static_cast<size_t>(std::lower_bound(v.begin(), v.end(), 1.0) - v.begin()),
                 static_cast<size_t>(std::upper_bound(v.begin(), v.end(), 1.0) - v.begin()) };
    }

    template<bool ROOT>
    void merge_mul(const std::vector<double>& large, const std::vector<double>& small, double to_find,
                   FormulaData& best, const size_t first_depth) {
        OpTimer t("Mul", ROOT);
        const size_t N = large.size(), M = small.size();
        const auto l = sign_split(large), s = sign_split(small);
        merge_region<Utils::MUL, +1, +1>(large, small, l.pos, N, s.pos, M, to_find, best, first_depth); // a>0,b>0
        merge_region<Utils::MUL, +1, -1>(large, small, 0, l.neg, s.pos, M, to_find, best, first_depth); // a<0,b>0
        merge_region<Utils::MUL, -1, +1>(large, small, l.pos, N, 0, s.neg, to_find, best, first_depth); // a>0,b<0
        merge_region<Utils::MUL, -1, -1>(large, small, 0, l.neg, 0, s.neg, to_find, best, first_depth); // a<0,b<0
        if (l.neg < l.pos) { update_best(0.0, large[l.neg], small.front(), Utils::MUL, first_depth, to_find, best); }
        if (s.neg < s.pos) { update_best(0.0, large.front(), small[s.neg], Utils::MUL, first_depth, to_find, best); }
    }

    template<bool ROOT>
    void merge_div1(const std::vector<double>& large, const std::vector<double>& small, double to_find,
                    FormulaData& best, const size_t first_depth) {
        OpTimer t("Div1", ROOT);
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
    }

    template<bool ROOT>
    void merge_div2(const std::vector<double>& large, const std::vector<double>& small, double to_find,
                    FormulaData& best, const size_t first_depth) {
        OpTimer t("Div2", ROOT);
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
    }

    // pow(a, b): base a = large (split at 1, must be > 0), exponent b = small (split at 0).
    // d/da = b*a^(b-1) so SA = sign(b); d/db = a^b*ln(a) so SB = sign(a - 1).
    template<bool ROOT>
    void merge_pow1(const std::vector<double>& large, const std::vector<double>& small, double to_find,
                    FormulaData& best, const size_t first_depth) {
        OpTimer t("pow1", ROOT);
        const size_t N = large.size(), M = small.size();
        const auto a = pos_split(large);
        const auto b = sign_split(small);
        merge_region<Utils::POW1, +1, +1>(large, small, a.oneHi, N, b.pos, M, to_find, best, first_depth); // a>1,b>0
        merge_region<Utils::POW1, +1, -1>(large, small, a.pos, a.oneLo, b.pos, M, to_find, best, first_depth); // 0<a<1,b>0
        merge_region<Utils::POW1, -1, +1>(large, small, a.oneHi, N, 0, b.neg, to_find, best, first_depth); // a>1,b<0
        merge_region<Utils::POW1, -1, -1>(large, small, a.pos, a.oneLo, 0, b.neg, to_find, best, first_depth); // 0<a<1,b<0
        if (a.oneLo < a.oneHi) { update_best(1.0, large[a.oneLo], small.front(), Utils::POW1, first_depth, to_find, best); } // 1^b = 1
        if (b.neg < b.pos && a.pos < N) { update_best(1.0, large[a.pos], small[b.neg], Utils::POW1, first_depth, to_find, best); } // a^0 = 1
    }

    // pow(b, a): base b = small (split at 1, must be > 0), exponent a = large (split at 0).
    // d/da = b^a*ln(b) so SA = sign(b - 1); d/db = a*b^(a-1) so SB = sign(a).
    template<bool ROOT>
    void merge_pow2(const std::vector<double>& large, const std::vector<double>& small, double to_find,
                    FormulaData& best, const size_t first_depth) {
        OpTimer t("pow2", ROOT);
        const size_t N = large.size(), M = small.size();
        const auto a = sign_split(large);
        const auto b = pos_split(small);
        merge_region<Utils::POW2, +1, +1>(large, small, a.pos, N, b.oneHi, M, to_find, best, first_depth); // b>1,a>0
        merge_region<Utils::POW2, +1, -1>(large, small, 0, a.neg, b.oneHi, M, to_find, best, first_depth); // b>1,a<0
        merge_region<Utils::POW2, -1, +1>(large, small, a.pos, N, b.pos, b.oneLo, to_find, best, first_depth); // 0<b<1,a>0
        merge_region<Utils::POW2, -1, -1>(large, small, 0, a.neg, b.pos, b.oneLo, to_find, best, first_depth); // 0<b<1,a<0
        if (b.oneLo < b.oneHi) { update_best(1.0, large.front(), small[b.oneLo], Utils::POW2, first_depth, to_find, best); } // 1^a = 1
        if (a.neg < a.pos && b.pos < M) { update_best(1.0, large[a.neg], small[b.pos], Utils::POW2, first_depth, to_find, best); } // b^0 = 1
    }

    // log(a)/log(b) = log_b(a): a = large, b = small, both split at 1 and required > 0 (b != 1).
    // SA = sign(b - 1); SB = -sign(a - 1).
    template<bool ROOT>
    void merge_log1(const std::vector<double>& large, const std::vector<double>& small, double to_find,
                    FormulaData& best, const size_t first_depth) {
        OpTimer t("log1", ROOT);
        const size_t N = large.size(), M = small.size();
        const auto a = pos_split(large), b = pos_split(small);
        merge_region<Utils::LOG1, +1, -1>(large, small, a.oneHi, N, b.oneHi, M, to_find, best, first_depth); // a>1,b>1
        merge_region<Utils::LOG1, +1, +1>(large, small, a.pos, a.oneLo, b.oneHi, M, to_find, best, first_depth); // 0<a<1,b>1
        merge_region<Utils::LOG1, -1, -1>(large, small, a.oneHi, N, b.pos, b.oneLo, to_find, best, first_depth); // a>1,0<b<1
        merge_region<Utils::LOG1, -1, +1>(large, small, a.pos, a.oneLo, b.pos, b.oneLo, to_find, best, first_depth); // 0<a<1,0<b<1
        if (a.oneLo < a.oneHi) { // log_b(1) = 0 for any valid base b != 1.
            if (b.oneHi < M) { update_best(0.0, large[a.oneLo], small[b.oneHi], Utils::LOG1, first_depth, to_find, best); }
            else if (b.pos < b.oneLo) { update_best(0.0, large[a.oneLo], small[b.pos], Utils::LOG1, first_depth, to_find, best); }
        }
    }

    // log(b)/log(a) = log_a(b): a = large, b = small, both split at 1 and required > 0 (a != 1).
    // SA = -sign(b - 1); SB = sign(a - 1).
    template<bool ROOT>
    void merge_log2(const std::vector<double>& large, const std::vector<double>& small, double to_find,
                    FormulaData& best, const size_t first_depth) {
        OpTimer t("log2", ROOT);
        const size_t N = large.size(), M = small.size();
        const auto a = pos_split(large), b = pos_split(small);
        merge_region<Utils::LOG2, -1, +1>(large, small, a.oneHi, N, b.oneHi, M, to_find, best, first_depth); // a>1,b>1
        merge_region<Utils::LOG2, +1, +1>(large, small, a.oneHi, N, b.pos, b.oneLo, to_find, best, first_depth); // a>1,0<b<1
        merge_region<Utils::LOG2, -1, -1>(large, small, a.pos, a.oneLo, b.oneHi, M, to_find, best, first_depth); // 0<a<1,b>1
        merge_region<Utils::LOG2, +1, -1>(large, small, a.pos, a.oneLo, b.pos, b.oneLo, to_find, best, first_depth); // 0<a<1,0<b<1
        if (b.oneLo < b.oneHi) { // log_a(1) = 0 for any valid base a != 1.
            if (a.oneHi < N) { update_best(0.0, large[a.oneHi], small[b.oneLo], Utils::LOG2, first_depth, to_find, best); }
            else if (a.pos < a.oneLo) { update_best(0.0, large[a.pos], small[b.oneLo], Utils::LOG2, first_depth, to_find, best); }
        }
    }

    // A reconstructed formula. A leaf carries an atom's symbol; an internal node carries an operator and its two
    // operands in canonical (operand1, operand2) order -- the same order FormulaData stores them, so value can be
    // recomputed with Utils::apply_runtime. Display reordering (e.g. b - a for SUB2) lives entirely in render().
    struct Formula {
        std::string label;                       // set iff this is a leaf
        Utils::Op op{};                          // operator, for internal nodes
        double value{};                          // value this subtree evaluates to
        std::unique_ptr<Formula> first, second;  // operand1, operand2 subtrees
        bool is_leaf() const { return first == nullptr; }
    };

    // Reconstruction reaches a depth-1 operand whose value is exactly one of the atoms; return that atom as a leaf.
    Formula exact_depth_one_match(double to_find) const {
        for (const auto& [value, label] : atoms) {
            if (value == to_find) {
                Formula leaf; leaf.label = label; leaf.value = value;
                return leaf;
            }
        }
        assert(false);
        return {};
    }

    // Top-level depth-1 search: no exact match guaranteed, so return the closest atom as a leaf.
    Formula find_depth_one(double to_find) const {
        const std::pair<double, std::string>* best = nullptr;
        double high_score = std::abs(to_find);
        for (const auto& atom : atoms) {
            if (best == nullptr || std::abs(to_find - atom.first) < high_score) {
                high_score = std::abs(to_find - atom.first);
                best = &atom;
            }
        }
        Formula leaf; leaf.label = best->second; leaf.value = best->first;
        return leaf;
    }

    // Operator binding strength for minimal parenthesization (higher binds tighter). Leaves and the log_b(a) form
    // are self-delimiting, so they rank as atomic and never need parentheses around them.
    static int precedence(const Formula& f) {
        if (f.is_leaf()) { return 100; }
        switch (f.op) {
            case Utils::ADD: case Utils::SUB1: case Utils::SUB2: return 1;
            case Utils::MUL: case Utils::DIV1: case Utils::DIV2: return 2;
            case Utils::POW1: case Utils::POW2: return 3;
            default: return 100; // LOG1 / LOG2
        }
    }

    static void render_child(const Formula& child, int parent_prec, bool is_left, bool parent_right_assoc,
                             std::string& out) {
        const int cp = precedence(child);
        // A child binding looser than its parent needs parentheses; at equal precedence, the side that would
        // re-associate the wrong way does (right operand of a left-associative op, left operand of a right one).
        const bool paren = cp < parent_prec || (cp == parent_prec && (parent_right_assoc ? is_left : !is_left));
        if (paren) { out += '('; }
        render(child, out);
        if (paren) { out += ')'; }
    }

    // log_b(a): base b on the subscript, argument a in parentheses. The base is wrapped when it is itself compound.
    static void render_log(const Formula& base, const Formula& arg, std::string& out) {
        out += "log_";
        const bool wrap = !base.is_leaf();
        if (wrap) { out += '('; }
        render(base, out);
        if (wrap) { out += ')'; }
        out += '(';
        render(arg, out);
        out += ')';
    }

    // Render a formula tree as parenthesized infix into out.
    static void render(const Formula& f, std::string& out) {
        if (f.is_leaf()) { out += f.label; return; }
        if (f.op == Utils::LOG1) { render_log(*f.second, *f.first, out); return; } // log_b(a): base=operand2
        if (f.op == Utils::LOG2) { render_log(*f.first, *f.second, out); return; } // log_a(b): base=operand1

        const Formula* L; const Formula* R; char sym; bool right_assoc = false;
        switch (f.op) {
            case Utils::ADD:  L = f.first.get();  R = f.second.get(); sym = '+'; break;
            case Utils::SUB1: L = f.first.get();  R = f.second.get(); sym = '-'; break;
            case Utils::SUB2: L = f.second.get(); R = f.first.get();  sym = '-'; break; // b - a
            case Utils::MUL:  L = f.first.get();  R = f.second.get(); sym = '*'; break;
            case Utils::DIV1: L = f.first.get();  R = f.second.get(); sym = '/'; break;
            case Utils::DIV2: L = f.second.get(); R = f.first.get();  sym = '/'; break; // b / a
            case Utils::POW1: L = f.first.get();  R = f.second.get(); sym = '^'; right_assoc = true; break;
            case Utils::POW2: L = f.second.get(); R = f.first.get();  sym = '^'; right_assoc = true; break; // b ^ a
            default: return; // unreachable: logs handled above
        }
        const int p = precedence(f);
        render_child(*L, p, true, right_assoc, out);
        out += ' '; out += sym; out += ' ';
        render_child(*R, p, false, right_assoc, out);
    }

    // True when the rebuilt subtree reproduces the value the search reported for it (relative tolerance for scale).
    static bool reproduces(double got, double want) {
        return std::abs(got - want) <= 1e-9 * std::max(1.0, std::abs(want));
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
    Formula findAndPrint(size_t depth, std::vector<std::vector<double>> &sources, double to_find) {
        if (depth == 1) {
            // A depth-1 root search might not land on an exact atom; reconstruction always does.
            return ROOT ? find_depth_one(to_find) : exact_depth_one_match(to_find);
        }
        SimpleClock depth_clock; // Times this depth's search; only read for the top-level (ROOT) call.
        if constexpr (ROOT) {
            depth_clock.start();
        }
        FormulaData best{};
        best.absolute_difference = std::abs(to_find) + 10;
        size_t larger_depth = depth - 1;
        if (larger_depth >= sources.size()) {
            LOG_AT(LogLevel::INFO) << "Jumping from search depth " << larger_depth << " down to " << sources.size() - 1 << std::endl;
            larger_depth = sources.size() - 1;
        }
        while (larger_depth * 2 >= depth) {
            const std::vector<double>& large = sources[larger_depth];
            const std::vector<double>& small = sources[depth - larger_depth];
            merge_full<ROOT, Utils::ADD, +1, +1>(large, small, to_find, best, larger_depth);   // a+b
            merge_full<ROOT, Utils::SUB1, +1, -1>(large, small, to_find, best, larger_depth);  // a-b
            merge_full<ROOT, Utils::SUB2, -1, +1>(large, small, to_find, best, larger_depth);  // b-a

            merge_mul<ROOT>(large, small, to_find, best, larger_depth);
            merge_div1<ROOT>(large, small, to_find, best, larger_depth);
            merge_div2<ROOT>(large, small, to_find, best, larger_depth);
            merge_pow1<ROOT>(large, small, to_find, best, larger_depth);
            merge_pow2<ROOT>(large, small, to_find, best, larger_depth);
            merge_log1<ROOT>(large, small, to_find, best, larger_depth);
            merge_log2<ROOT>(large, small, to_find, best, larger_depth);

            larger_depth--;
        }
        // Timing is measured over the search only; reconstruction below re-searches the (cheap, shallow) operands.
        double seconds = 0;
        if constexpr (ROOT) { seconds = depth_clock.end(); }

        Formula node;
        node.op = best.operation;
        node.first  = std::make_unique<Formula>(findAndPrint<false>(best.depth1, sources, best.operand1));
        node.second = std::make_unique<Formula>(findAndPrint<false>(depth - best.depth1, sources, best.operand2));
        node.value  = Utils::apply_runtime(best.operation, node.first->value, node.second->value);
        // The rebuilt operands must reproduce the doubles the search picked, or the printed formula would lie.
        assert(reproduces(node.first->value, best.operand1) && reproduces(node.second->value, best.operand2));

        if constexpr (ROOT) {
            assert(reproduces(std::abs(to_find - node.value), best.absolute_difference));
            LOG_AT(LogLevel::INFO) << "depth " << depth << " searched in " << seconds << " seconds" << std::endl;
            std::string formula;
            render(node, formula);
            LOG_AT(LogLevel::RESULT) << "depth " << depth << ": high score " << best.absolute_difference
                                     << "; x = " << formula << std::endl;
        }
        return node;
    }

    explicit MergeFinder(const std::vector<std::pair<double, std::string>>& atoms)
        : atoms(atoms) {
    }

private:
    const std::vector<std::pair<double, std::string>>& atoms; // depth-1 {value, symbol} pairs, for printing leaves.

    static constexpr std::string_view op_strings[] = {[Utils::ADD] = "+", [Utils::SUB1] = "-", [Utils::SUB2] = "-",
                                [Utils::MUL] = "*", [Utils::DIV1] = "/", [Utils::DIV2] = "/", [Utils::POW1] = "pow1",
                                [Utils::POW2] = "pow2", [Utils::LOG1] = "log1", [Utils::LOG2] = "log2" };
};
