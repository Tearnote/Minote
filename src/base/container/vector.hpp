#pragma once

#include "absl/container/inlined_vector.h"
#include "base/memory/nullalloc.hpp"
#include "base/types.hpp"

namespace minote::base {

// Inlined vector. Stored on stack initially, switches to heap above N elements.
template<typename T, usize N = 256, template<typename> typename Allocator = std::allocator>
using ivector = absl::InlinedVector<T, N, Allocator<T>>;

// Static vector. Stored entirely on stack, throws if capacity is exceeded.
template<typename T, usize N>
using svector = ivector<T, N, NullAllocator>;

}
