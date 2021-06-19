#pragma once

#include "base/container/ivector.hpp"
#include "base/memory/nullalloc.hpp"
#include "base/types.hpp"

namespace minote::base {

template<typename T, usize N>
using svector = ivector<T, N, NullAllocator>;

}
