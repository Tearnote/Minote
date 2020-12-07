// Minote - base/array.hpp
// Container types for working on contiguous memory

#pragma once

#include <vector>
#include <array>
#include <span>
#include "itlib/static_vector.hpp"
#include "base/util.hpp"

namespace minote {

// Standard array - size set at compile time
using std::array;

// Static vector - allocated on stack, capacity limit set at compile time
template<typename T, size_t Capacity>
using svector = itlib::static_vector<T, Capacity>;

// Dynamic vector - reallocates memory on growth
template<typename T>
using vector = std::vector<T>;

// A slice of any contiguous array type
using std::span;

}
