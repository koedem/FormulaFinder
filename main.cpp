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

/**
 * TODO add square root resolution
 * @param root
 * @param depth
 * @param sources
 * @param to_find
 */
void findAndPrint(bool root, size_t depth, std::vector<std::vector<double>> &sources, double to_find) {
    SimpleClock clock2;
    if (root) {
        clock1.start();
    }
    double highscore = abs(to_find) + 10;
    double operand1;
    if (depth == 1) {
        for (double x : sources[1]) {
            if (abs(to_find - x) < highscore) {
                highscore = abs(to_find - x);
                operand1 = x;
            }
        }
        std::cout << operand1 << " ";
        return;
    }

    std::string best_op;
    size_t larger_depth = depth - 1, depth1;
    double operand2;
    if (larger_depth >= sources.size()) {
        std::cout << "Jumping from search depth " << larger_depth << " down to " << sources.size() - 1 << std::endl;
        larger_depth = sources.size();
    }
    while (larger_depth * 2 >= depth) {
        const std::vector<double>& large = sources[larger_depth];
        const std::vector<double>& small = sources[depth - larger_depth];
        if (root) {
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
        if (root) {
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
        if (root) {
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
        if (root) {
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
            if (root) {
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
            if (root) {
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
            if (root) {
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
            if (root) {
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
            if (root) {
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

            if (root) {
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
            if (root) {
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
            if (root) {
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
        if (root) {
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
        if (root) {
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
        if (root) {
            std::cout << "Slow Log2 " << clock2.end() << std::endl;
            clock2.start();
        }
        larger_depth--;
    }
    if (root) {
        std::cout << clock1.end() << " seconds, depth: " << depth << ", highscore: " << highscore << "; x =  ";
    }
    std::cout << best_op << " ";
    findAndPrint(false, depth1, sources, operand1);
    findAndPrint(false, depth - depth1, sources, operand2);
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

void count_matching_negative_numbers(const std::vector<double> values) {
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
        findAndPrint(true, depth, values_per_depth, to_find);
        std::cout << std::endl;

        generate_values(values_per_depth, depth);
        count_matching_negative_numbers(values_per_depth[depth]);
    }

    findAndPrint(true, 7, values_per_depth, to_find);
    std::cout << std::endl;
    findAndPrint(true, 8, values_per_depth, to_find);
    std::cout << std::endl;
    return 0;
}
/*
Adding 6.12e-06
Sub1 2e-07
Sub2 2.4e-07
Mul 2.2e-07
Div1 2e-07
Slow Div2 2.7e-07
Slow Pow1 1.126e-05
Slow Pow2 1.15e-06
Slow Log1 1.54e-06
Slow Log2 9.2e-07
5.115e-05 seconds, depth: 2, highscore: 0.0115499; x =  log 8 3
Start generation.
7.64e-06 seconds, end generation.
Start pruning.
3.05e-06 seconds, pruned 820 to 182
Adding 2.7e-07
Sub1 2.4e-07
Sub2 3e-07
Mul 3.7e-07
Div1 3.3e-07
Slow Div2 1.79e-06
Slow Pow1 2.021e-05
Slow Pow2 1.751e-05
Slow Log1 1.533e-05
Slow Log2 1.428e-05
8.471e-05 seconds, depth: 3, highscore: 4.59955e-05; x =  log * 5 10 8
Start generation.
0.00012979 seconds, end generation.
Start pruning.
5.238e-05 seconds, pruned 14924 to 6903
Adding 3.13e-06
Sub1 3.96e-06
Sub2 4.08e-06
Mul 2.13e-06
Div1 3.32e-06
Slow Div2 6.556e-05
Slow Pow1 0.0007132
Slow Pow2 0.00074347
Slow Log1 0.00055482
Slow Log2 0.00053894
Adding 1.36e-06
Sub1 1.13e-06
Sub2 1.29e-06
Mul 1.45e-06
Div1 2.49e-06
Slow Div2 3.151e-05
Slow Pow1 0.000340449
Slow Pow2 0.00033887
Slow Log1 0.00027178
Slow Log2 0.00026217
0.00391518 seconds, depth: 4, highscore: 4.59955e-05; x =  log + * 4 10 10 8
Start generation.
0.0055587 seconds, end generation.
Start pruning.
0.00293028 seconds, pruned 566046 to 295490
Start generation.
0.00254172 seconds, end generation.
Start pruning.
0.00185775 seconds, pruned 560846 to 332311
Adding 0.00014306
Sub1 0.00018486
Sub2 0.00017405
Mul 9.638e-05
Div1 0.00013813
Slow Div2 0.00321915
Slow Pow1 0.0351705
Slow Pow2 0.0333525
Slow Log1 0.0271637
Slow Log2 0.0278708
Adding 7.04e-06
Sub1 9.57e-06
Sub2 9.43e-06
Mul 1.051e-05
Div1 9.15e-06
Slow Div2 0.0012438
Slow Pow1 0.0134581
Slow Pow2 0.0144349
Slow Log1 0.0105845
Slow Log2 0.0104995
0.177832 seconds, depth: 5, highscore: 6.13021e-07; x =  log 6 - / 9 / 8 5 3
Start generation.
0.294474 seconds, end generation.
Start pruning.
0.136496 seconds, pruned 27249502 to 13784076
Start generation.
0.141224 seconds, end generation.
Start pruning.
0.120083 seconds, pruned 23848650 to 16490056
Adding 0.00793389
Sub1 0.010027
Sub2 0.00959094
Mul 0.00534052
Div1 0.00726504
Slow Div2 0.166593
Slow Pow1 1.78066
Slow Pow2 1.64225
Slow Log1 1.32431
Slow Log2 1.34282
Adding 0.00019497
Sub1 0.00029924
Sub2 0.00029734
Mul 0.00034365
Div1 0.00025568
Slow Div2 0.0599549
Slow Pow1 0.652459
Slow Pow2 0.669267
Slow Log1 0.505214
Slow Log2 0.502439
Adding 4.536e-05
Sub1 4.764e-05
Sub2 4.681e-05
Mul 4.939e-05
Div1 7.864e-05
Slow Div2 0.046538
Slow Pow1 0.53226
Slow Pow2 0.529112
Slow Log1 0.393903
Slow Log2 0.391349
10.5812 seconds, depth: 6, highscore: 2.83025e-09; x =  log * 7 10 - * log 8 3 8 6
Start generation.
13.2465 seconds, end generation.
Start pruning.
6.81372 seconds, pruned 1352184592 to 684079076
Start generation.
5.4156 seconds, end generation.
Start pruning.
6.45887 seconds, pruned 1168588514 to 814231106
Start generation.
4.87126 seconds, end generation.
Start pruning.
6.43607 seconds, pruned 1195456184 to 856971635
200316325 1379338
Adding 0.433064
Sub1 0.535018
Sub2 0.51332
Mul 0.285033
Div1 0.377391
Slow Div2 8.69706
Slow Pow1 96.7899
Slow Pow2 88.8157
Slow Log1 70.9874
Slow Log2 70.0047
Adding 0.0185185
Sub1 0.0188665
Sub2 0.0146186
Mul 0.0190233
Div1 0.0121579
Slow Div2 2.98297
Slow Pow1 33.0063
Slow Pow2 32.7177
Slow Log1 25.2112
Slow Log2 25.1346
Adding 0.00038883
Sub1 0.000420889
Sub2 0.00042371
Mul 0.00042002
Div1 0.00039075
Slow Div2 2.3033
Slow Pow1 26.3394
Slow Pow2 25.0383
Slow Log1 19.1137
Slow Log2 18.8197
548.192 seconds, depth: 7, highscore: 6.12068e-10; x =  / pow 10 6 + pow / 10 3 4 pow 9 6
Jumping from search depth 7 down to 6
Adding 0.538014
Sub1 0.761384
Sub2 0.75567
Mul 0.868616
Div1 0.610982
Slow Div2 154.764
Slow Pow1 1744.51
Slow Pow2 1719.1
Slow Log1 1325.41
Slow Log2 1308.37
Adding 0.0147905
Sub1 0.016997
Sub2 0.0169365
Mul 0.017157
Div1 0.0147228
Slow Div2 111.686
Slow Pow1 1297.99
Slow Pow2 1193.16

 */