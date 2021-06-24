#pragma once

#include <array>
#include "absl/container/fixed_array.h"
#include "base/memory/nullalloc.hpp"
#include "base/types.hpp"

namespace minote::base {

// An array of size declared at runtime in the constructor. Stored on stack
// or heap, depending on total size. Elements are not initialized.
template<typename T, template<typename> typename Allocator = std::allocator>
using array = absl::FixedArray<T, absl::kFixedArrayUseDefault, Allocator<T>>;

// Static array. Stored entirely on stack, with size provided at runtime.
// Elements are not initialized.
template<typename T, usize N>
using sarray = std::array<T, N>;

}
