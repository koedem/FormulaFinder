#pragma once

#include "Utils.h"
#include "OpMerger.h"

template<> size_t starting_i<Utils::ADD>(size_t large_size) {
    return 0;
}
template<> size_t starting_j<Utils::ADD>(size_t small_size) {
    return small_size - 1;
}
template<> void update_big_x<Utils::ADD>(size_t& i, size_t& j) {
    j--;
}

template<> void update_small_x<Utils::ADD>(size_t& i, size_t& j) {
    i++;
}
template<> bool condition<Utils::ADD>(size_t& i, size_t& j, size_t large_size, size_t small_size) {
    return i < large_size && j <= small_size;
}