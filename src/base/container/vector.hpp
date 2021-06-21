#pragma once

#include <vector>

namespace minote::base {

// Standard vector, reallocates if capacity is exceeded.
template<typename T, template<typename> typename Allocator = std::allocator>
using vector = std::vector<T, Allocator<T>>;

}
