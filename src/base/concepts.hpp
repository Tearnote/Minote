#pragma once

#include <type_traits>
#include <concepts>

namespace minote::base {

// Any enum or enum struct
template<typename T>
concept enum_type = std::is_enum_v<T>;

// Built-in type with defined arithmetic operations (+, -, *, /)
template<typename T>
concept arithmetic = std::is_arithmetic_v<T>;

// Objects that can be safely copied with memcpy.
template<typename T>
concept trivially_copyable = std::is_trivially_copyable_v<T>;

// Imports of built-in concepts

using std::default_initializable;
using std::floating_point;
using std::invocable;
using std::integral;

}
