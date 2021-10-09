#pragma once

#include <type_traits>
#include <tuple>

namespace minote::base {

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

// Constrain to a type that can be held within a tuple/variant/etc.

template <typename T, typename Tuple>
struct contains_type;

template <typename T, typename... Us>
struct contains_type<T, std::tuple<Us...>>: std::disjunction<std::is_same<T, Us>...> {};

template<typename T, typename U>
concept contains = contains_type<U, T>::value;

}
