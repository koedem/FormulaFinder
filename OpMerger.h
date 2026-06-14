#pragma once


#include "Utils.h"

template<Utils::Op> size_t starting_i(size_t large_size);
template<Utils::Op> size_t starting_j(size_t small_size);
template<Utils::Op> void update_big_x(size_t& i, size_t& j);
template<Utils::Op> void update_small_x(size_t& i, size_t& j);
template<Utils::Op> bool condition(size_t& i, size_t& j, size_t large_size, size_t small_size);
