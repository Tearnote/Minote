#pragma once

#include "absl/container/fixed_array.h"

namespace minote::base {

template<typename T, template<typename> typename Allocator>
using array = absl::FixedArray<T, absl::kFixedArrayUseDefault, Allocator<T>>;

}
