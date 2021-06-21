#pragma once

#include "absl/container/fixed_array.h"

namespace minote::base {

// An array of size declared at runtime in the constructor. Stored on stack
// or heap, depending on total size. Elements are not initialized.
template<typename T, template<typename> typename Allocator = std::allocator>
using array = absl::FixedArray<T, absl::kFixedArrayUseDefault, Allocator<T>>;

}
