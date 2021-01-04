#pragma once

#include <type_traits>

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

}
