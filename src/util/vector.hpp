#pragma once

#include "itlib/static_vector.hpp"
#include "itlib/small_vector.hpp"
#include "itlib/pod_vector.hpp"
#include "util/types.hpp"

namespace minote {

// Static vector. Stored entirely on stack, throws if capacity is exceeded.
template<typename T, usize N>
using svector = itlib::static_vector<T, N>;

// Inlined vector. Stored on stack initially, switches to heap above N elements.
template<typename T, usize N = 16>
using ivector = itlib::small_vector<T, N>;

// POD vector. Only usable with POD types, but greatly optimized - raw memory
// operations can be used, and construction/destruction is avoided.
template<typename T>
using pvector = itlib::pod_vector<T>;

}
