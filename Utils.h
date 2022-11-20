#pragma once

#include <vector>

class Utils {

public:

    /**
     * This function searches a vector for the occurrence of a value.
     * If the value is contained in the vector, its index will be returned.
     * If the value is not contained in the vector, the index of the next smaller element is returned,
     * unless the value is smaller than the smallest value, in which case 0 is returned.
     * Runs in log(sorted.size()).
     * @param sorted a vector of values in ascending order to be searched.
     * @param to_find the value to be found.
     * @return the index of the best match value in the vector.
     */
    static size_t approxBinSearch(const std::vector<double> &sorted, double to_find) {
        if (to_find < sorted[0]) {
            return 0;
        } else if (to_find > sorted.back()) {
            return sorted.size() - 1;
        } else {
            size_t low = 0, high = sorted.size() - 1;
            while (low < high - 1) {
                size_t middle = (low + high) / 2;
                if (sorted[middle] > to_find) {
                    high = middle;
                } else {
                    low = middle;
                }
            }
            return low;
        }
    }

    template<size_t OP>
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

    enum Op { ADD = 0, SUB1 = 1, SUB2 = 2, MUL = 3, DIV1 = 4, DIV2 = 5, POW1 = 6, POW2 = 7, LOG1 = 8, LOG2 = 9 };

private:
    Utils() = default;
};
