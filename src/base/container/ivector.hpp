#pragma once

#include "absl/container/inlined_vector.h"
#include "base/types.hpp"

namespace minote::base {

template<typename T, usize N, template<typename> typename Allocator>
using ivector = absl::InlinedVector<T, N, Allocator<T>>;

}
