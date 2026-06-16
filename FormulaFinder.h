#pragma once

#include <cassert>
#include <cmath>
#include <memory>
#include <string>
#include <utility>
#include <vector>
#include "Formula.h"
#include "MergeFinder.h"
#include "Operators.h"
#include "Real.h"
#include "SimpleClock.h"
#include "Log.h"

/**
 * Drives the search: for a target value it asks MergeFinder for the closest formula at a given depth, reconstructs
 * that result into a Formula tree (recursively re-searching each operand), and at the top level times and prints it.
 * This is the layer that knows about atoms and seeded constants -- the leaves a tree bottoms out in.
 */
class FormulaFinder {
public:
    FormulaFinder(const std::vector<std::pair<Real, std::string>>& atoms,
                  const std::vector<std::pair<Real, std::string>>& constants)
        : atoms(atoms), constants(constants) {
    }

    /**
     * Searches for the closest formula of the given depth to to_find, then (at ROOT) reconstructs and prints it.
     * @param depth target expression depth.
     * @param sources value tiers, sources[k] holding every value reachable at depth k.
     * @param to_find the value to approximate.
     */
    template<bool ROOT>
    Formula findAndPrint(size_t depth, std::vector<std::vector<Real>>& sources, Real to_find) {
        if (depth == 1) {
            // A depth-1 root search might not land on an exact atom; reconstruction always does.
            return ROOT ? find_depth_one(to_find) : exact_depth_one_match(to_find);
        }
        if constexpr (!ROOT) {
            // A depth-2 operand may be a seeded constant: recognise it as a leaf instead of rebuilding it from atoms
            // (which would generally fail, since constants are not reachable from the depth-1 atoms).
            if (depth == 2) {
                if (const auto* c = closest_constant(to_find); c != nullptr && c->first == to_find) {
                    Formula leaf; leaf.label = c->second; leaf.value = c->first;
                    return leaf;
                }
            }
        }
        SimpleClock depth_clock; // Times this depth's search; only read for the top-level (ROOT) call.
        if constexpr (ROOT) {
            depth_clock.start();
        }
        MergeFinder::FormulaData best = MergeFinder::search<ROOT>(depth, sources, to_find);

        // A seeded constant lives at depth 2 but is not in sources yet when depth 2 is searched (it is added by the
        // following generate_values call), so the root depth-2 search compares against it explicitly. Deeper searches
        // pick constants up for free, as they are by then present in sources[2].
        const std::pair<Real, std::string>* winning_constant = nullptr;
        if constexpr (ROOT) {
            if (depth == 2) {
                if (const auto* c = closest_constant(to_find); c != nullptr
                        && std::abs(to_find - c->first) < best.absolute_difference) {
                    winning_constant = c;
                    best.absolute_difference = std::abs(to_find - c->first);
                }
            }
        }
        // Timing is measured over the search only; reconstruction below re-searches the (cheap, shallow) operands.
        double seconds = 0;
        if constexpr (ROOT) { seconds = depth_clock.end(); }

        Formula node;
        if (winning_constant != nullptr) { // a bare constant beat every binary/sqrt combination
            node.label = winning_constant->second;
            node.value = winning_constant->first;
        } else {
            node.op = best.operation;
            node.first = std::make_unique<Formula>(findAndPrint<false>(best.depth1, sources, best.operand1));
            if (best.operation == Operators::SQRT) { // unary: no second operand, value is sqrt of the one child
                node.value = Operators::apply_runtime(Operators::SQRT, node.first->value, 0.0);
                assert(reproduces(node.first->value, best.operand1));
            } else {
                node.second = std::make_unique<Formula>(findAndPrint<false>(depth - best.depth1, sources, best.operand2));
                node.value  = Operators::apply_runtime(best.operation, node.first->value, node.second->value);
                // The rebuilt operands must reproduce the doubles the search picked, or the printed formula would lie.
                assert(reproduces(node.first->value, best.operand1) && reproduces(node.second->value, best.operand2));
            }
        }

        if constexpr (ROOT) {
            assert(reproduces(std::abs(to_find - node.value), best.absolute_difference));
            LOG_AT(LogLevel::INFO) << "depth " << depth << " searched in " << seconds << " seconds" << std::endl;
            const std::string formula = FormulaRenderer::render(node);
            LOG_AT(LogLevel::RESULT) << "depth " << depth << ": high score " << best.absolute_difference
                                     << "; x = " << formula << std::endl;
        }
        return node;
    }

private:
    // Reconstruction reaches a depth-1 operand whose value is exactly one of the atoms; return that atom as a leaf.
    Formula exact_depth_one_match(Real to_find) const {
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
    Formula find_depth_one(Real to_find) const {
        const std::pair<Real, std::string>* best = nullptr;
        Real high_score = std::abs(to_find);
        for (const auto& atom : atoms) {
            if (best == nullptr || std::abs(to_find - atom.first) < high_score) {
                high_score = std::abs(to_find - atom.first);
                best = &atom;
            }
        }
        Formula leaf; leaf.label = best->second; leaf.value = best->first;
        return leaf;
    }

    // Closest seeded depth-2 constant to to_find, or nullptr if no constants are configured. Constants are extra
    // building blocks (e.g. the golden ratio) that live only at depth 2, so this is consulted only there.
    const std::pair<Real, std::string>* closest_constant(Real to_find) const {
        const std::pair<Real, std::string>* best = nullptr;
        Real dist = 0;
        for (const auto& c : constants) {
            const Real d = std::abs(to_find - c.first);
            if (best == nullptr || d < dist) { dist = d; best = &c; }
        }
        return best;
    }

    // True when the rebuilt subtree reproduces the value the search reported for it (relative tolerance for scale).
    static bool reproduces(Real got, Real want) {
        return std::abs(got - want) <= 1e-9 * std::max(Real(1.0), std::abs(want));
    }

    const std::vector<std::pair<Real, std::string>>& atoms;     // depth-1 {value, symbol} pairs, for printing leaves.
    const std::vector<std::pair<Real, std::string>>& constants; // depth-2 {value, symbol} pairs, the seeded constants.
};