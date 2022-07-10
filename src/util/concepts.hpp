#pragma once

#include <type_traits>
#include <concepts>

namespace minote {

using std::same_as;

// Any enum or enum struct
template<typename T>
concept enum_type = std::is_enum_v<T>;

// Built-in type with defined arithmetic operations (+, -, *, /)
template<typename T>
concept arithmetic = std::is_arithmetic_v<T>;

using std::integral;
using std::floating_point;

template<typename T>
concept trivially_copyable = std::is_trivially_copyable_v<T>;

using std::default_initializable;

using std::invocable;
using std::predicate;

}
