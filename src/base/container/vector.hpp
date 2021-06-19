#pragma once

#include <vector>

namespace minote::base {

template<typename T, template<typename> typename Allocator>
using vector = std::vector<T, Allocator<T>>;

}
