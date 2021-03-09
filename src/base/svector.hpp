#pragma once

#include "static_vector/fixed_capacity_vector"
#include "base/types.hpp"

namespace minote::base {

// Static vector - allocated on stack, capacity limit set at compile time
template<typename T, size_t Capacity>
using svector = std::experimental::fixed_capacity_vector<T, Capacity>;

}
