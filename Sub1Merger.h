#pragma once

#include "Utils.h"
#include "OpMerger.h"

template<> inline size_t starting_i<Utils::SUB1>(size_t large_size) {
    return 0;
}
template<> inline size_t starting_j<Utils::SUB1>(size_t small_size) {
    return 0;
}
template<> inline void update_big_x<Utils::SUB1>(size_t& i, size_t& j) {
    j++;
}

template<> inline void update_small_x<Utils::SUB1>(size_t& i, size_t& j) {
    i++;
}
template<> inline bool condition<Utils::SUB1>(size_t& i, size_t& j, size_t large_size, size_t small_size) {
    return i < large_size && j < small_size;
}
