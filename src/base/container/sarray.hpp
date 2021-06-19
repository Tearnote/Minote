#pragma once

#include "absl/container/fixed_array.h"
#include "base/memory/nullalloc.hpp"

namespace minote::base {

template<typename T, usize N>
using sarray = absl::FixedArray<T, N, NullAllocator<T>>;

}
