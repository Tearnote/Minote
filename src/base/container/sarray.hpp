#pragma once

#include "absl/container/fixed_array.h"
#include "base/memory/nullalloc.hpp"

namespace minote::base {

// Static array. Stored entirely on stack, with size provided at runtime.
// Elements are not initialized.
template<typename T, usize N>
using sarray = absl::FixedArray<T, N, NullAllocator<T>>;

}
