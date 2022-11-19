#pragma once

#include <vector>

class Utils {

public:
    static size_t approxBinSearch(const std::vector<double> &sorted, double to_find) {
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

private:
    Utils() = default;
};