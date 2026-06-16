#pragma once

#include <memory>
#include <string>
#include "Operators.h"
#include "Real.h"

/**
 * A reconstructed formula. A leaf carries an atom's symbol; an internal node carries an operator and its two
 * operands in canonical (operand1, operand2) order -- the same order the search stores them, so value can be
 * recomputed with Operators::apply_runtime. Display reordering (e.g. b - a for SUB2) lives entirely in the renderer.
 */
struct Formula {
    std::string label;                       // set iff this is a leaf
    Operators::Op op{};                      // operator, for internal nodes
    Real value{};                            // value this subtree evaluates to
    std::unique_ptr<Formula> first, second;  // operand1, operand2 subtrees
    bool is_leaf() const { return first == nullptr; }                        // no children
    bool is_unary() const { return first != nullptr && second == nullptr; }  // one child (e.g. sqrt)
};

/**
 * Renders a Formula tree as minimally-parenthesized infix text. Parentheses are emitted only where operator
 * precedence or associativity would otherwise change the meaning.
 */
class FormulaRenderer {
public:
    static std::string render(const Formula& f) {
        std::string out;
        render(f, out);
        return out;
    }

private:
    // Binding strength of a formula node: leaves are atomic (100); internal nodes defer to the operator's precedence.
    static int precedence(const Formula& f) {
        return f.is_leaf() ? 100 : Operators::precedence(f.op);
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

    static void render(const Formula& f, std::string& out) {
        if (f.is_leaf()) { out += f.label; return; }
        if (f.is_unary()) { out += "sqrt("; render(*f.first, out); out += ')'; return; } // only unary op is SQRT
        if (f.op == Operators::LOG1) { render_log(*f.second, *f.first, out); return; } // log_b(a): base=operand2
        if (f.op == Operators::LOG2) { render_log(*f.first, *f.second, out); return; } // log_a(b): base=operand1

        const Formula* L; const Formula* R; char sym; bool right_assoc = false;
        switch (f.op) {
            case Operators::ADD:  L = f.first.get();  R = f.second.get(); sym = '+'; break;
            case Operators::SUB1: L = f.first.get();  R = f.second.get(); sym = '-'; break;
            case Operators::SUB2: L = f.second.get(); R = f.first.get();  sym = '-'; break; // b - a
            case Operators::MUL:  L = f.first.get();  R = f.second.get(); sym = '*'; break;
            case Operators::DIV1: L = f.first.get();  R = f.second.get(); sym = '/'; break;
            case Operators::DIV2: L = f.second.get(); R = f.first.get();  sym = '/'; break; // b / a
            case Operators::POW1: L = f.first.get();  R = f.second.get(); sym = '^'; right_assoc = true; break;
            case Operators::POW2: L = f.second.get(); R = f.first.get();  sym = '^'; right_assoc = true; break; // b ^ a
            default: return; // unreachable: logs handled above
        }
        const int p = precedence(f);
        render_child(*L, p, true, right_assoc, out);
        out += ' '; out += sym; out += ' ';
        render_child(*R, p, false, right_assoc, out);
    }
};