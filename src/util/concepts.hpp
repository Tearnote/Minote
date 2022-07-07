#pragma once

#include <type_traits>
#include <concepts>
#include <tuple>

namespace minote {

// Any enum or enum struct
template<typename T>
concept enum_type = std::is_enum_v<T>;

// Built-in type with defined arithmetic operations (+, -, *, /)
template<typename T>
concept arithmetic = std::is_arithmetic_v<T>;

using std::integral;

// Objects that can be safely copied with memcpy.
template<typename T>
concept trivially_copyable = std::is_trivially_copyable_v<T>;

using std::invocable;

}
