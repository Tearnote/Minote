// Minote - base/array.hpp
// Container types for working on contiguous memory.

#pragma once

#include <vector>
#include <array>
#include <span>
#include "itlib/static_vector.hpp"
#include "base/util.hpp"

namespace minote {

using std::array;
template<typename T, size_t Capacity>
using vector = itlib::static_vector<T, Capacity>;
template<typename T>
using dvector = std::vector<T>;
using std::span;

}
