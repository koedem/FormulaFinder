#pragma once

#include <cmath>
#include <string_view>

/**
 * Centralized operator vocabulary: the operator set plus everything derived from it -- how each evaluates (both
 * compile-time and at run time), its printed symbol, and its infix binding strength. Every other layer (the search
 * engine, the formula tree, the renderer) refers back here, so an operator is defined in exactly one place.
 */
class Operators {
public:
    // SQRT is the lone unary operator: it uses only the first operand, so its second operand is meaningless.
    enum Op { ADD = 0, SUB1 = 1, SUB2 = 2, MUL = 3, DIV1 = 4, DIV2 = 5, POW1 = 6, POW2 = 7, LOG1 = 8, LOG2 = 9,
              SQRT = 10 };

    // Compile-time evaluation, selected by the operator template argument so the search's hot loops inline it.
    template<Op OP>
    static double apply_operator(double a, double b) {
        if constexpr (OP == ADD) {
            return a + b;
        } else if constexpr (OP == SUB1) {
            return a - b;
        } else if constexpr (OP == SUB2) {
            return b - a;
        } else if constexpr (OP == MUL) {
            return a * b;
        } else if constexpr (OP == DIV1) {
            return a / b;
        } else if constexpr (OP == DIV2) {
            return b / a;
        } else if constexpr (OP == POW1) {
            return pow(a, b);
        } else if constexpr (OP == POW2) {
            return pow(b, a);
        } else if constexpr (OP == LOG1) {
            return log(a) / log(b);
        } else if constexpr (OP == LOG2) {
            return log(b) / log(a);
        }
    }

    // Runtime counterpart of apply_operator, for evaluating a reconstructed formula tree whose operators are only
    // known at run time. Mirrors apply_operator exactly so a rebuilt tree reproduces the searched value.
    static double apply_runtime(Op op, double a, double b) {
        switch (op) {
            case ADD:  return a + b;
            case SUB1: return a - b;
            case SUB2: return b - a;
            case MUL:  return a * b;
            case DIV1: return a / b;
            case DIV2: return b / a;
            case POW1: return pow(a, b);
            case POW2: return pow(b, a);
            case LOG1: return log(a) / log(b);
            case LOG2: return log(b) / log(a);
            case SQRT: return sqrt(a); // unary: b is ignored
        }
        return 0.0; // unreachable
    }

    // Short label for each operator, used for the per-operation debug timing breakdown.
    static std::string_view symbol(Op op) {
        switch (op) {
            case ADD:  return "+";
            case SUB1: return "-";
            case SUB2: return "-";
            case MUL:  return "*";
            case DIV1: return "/";
            case DIV2: return "/";
            case POW1: return "pow1";
            case POW2: return "pow2";
            case LOG1: return "log1";
            case LOG2: return "log2";
            case SQRT: return "sqrt";
        }
        return "?"; // unreachable
    }

    // Infix binding strength for minimal parenthesization (higher binds tighter). LOG and SQRT are self-delimiting
    // (printed with their own brackets), so they rank as atomic and never need parentheses around them.
    static int precedence(Op op) {
        switch (op) {
            case ADD: case SUB1: case SUB2: return 1;
            case MUL: case DIV1: case DIV2: return 2;
            case POW1: case POW2: return 3;
            default: return 100; // LOG1 / LOG2 / SQRT
        }
    }

private:
    Operators() = default;
};