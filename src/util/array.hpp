#pragma once

#include <array>

namespace minote {

// Static array. Stored entirely on stack, with size provided at runtime
// Elements are not initialized if value-initializable
using std::array;
using std::to_array;

}
