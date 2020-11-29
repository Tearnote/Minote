// Minote - base/concept.hpp
// Basic concept library

#pragma once

#include <type_traits>
#include <concepts>

namespace minote {

template<typename T>
concept enum_type = std::is_enum_v<T>;

// Built-in floating-point type
using std::floating_point;

// Built-in integral (integer) type
using std::integral;

// Built-in type with defined arithmetic operations (+, -, *, /)
template<typename T>
concept arithmetic = std::is_arithmetic_v<T>;

// An instance can be created via copy construction
using std::copy_constructible;

// An instance can be constructed with zero arguments
using std::default_initializable;

}
