#pragma once

#include <array>
#include "base/types.hpp"

namespace minote::base {

// Static array. Stored entirely on stack, with size provided at runtime.
// Elements are not initialized if value-initializable.
template<typename T, usize N>
using array = std::array<T, N>;

using std::to_array;

}
