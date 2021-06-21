#pragma once

#include "base/container/ivector.hpp"
#include "base/memory/nullalloc.hpp"
#include "base/types.hpp"

namespace minote::base {

// Static vector. Stored entirely on stack, throws if capacity is exceeded.
template<typename T, usize N>
using svector = ivector<T, N, NullAllocator>;

}
