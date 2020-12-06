// Minote - base/util.hpp
// Basic and universally useful C++ utilities

#pragma once

#include <type_traits>
#include <functional>
#include <optional>
#include <utility>
#include <cstdint>
#include <cstdlib> // Provide free()
#include <cstring>
#include <cstdio>
#include "scope_guard/scope_guard.hpp"
#include "PPK_ASSERT/ppk_assert.h"
#include "base/concept.hpp"

namespace minote {

// *** Standard imports ***

// Basic types
using u8 = std::uint8_t;
using u16 = std::uint16_t;
using u32 = std::uint32_t;
using u64 = std::uint64_t;
using i8 = std::int8_t;
using i16 = std::int16_t;
using i32 = std::int32_t;
using i64 = std::int64_t;
using f32 = float;
using f64 = double;

// Used every time the size of a container is stored. Should be a keyword really
using std::size_t;

// Reference wrapper. Required to use references in thread constructor, containers, etc
using std::ref;
using std::cref;

// Used for returning by value when lack of value is possible
using std::optional;
using std::nullopt;

// Convert to rvalue reference. Used often for ownership transfer and insertion into containers
using std::move;

// Immediately close the application
using std::exit;

// *** Language features ***

// Make the DEFER feature from scope_guard more keyword-like
#define defer DEFER

// Improved assert() macro from PPK_ASSERT, with printf-like formatting
#define ASSERT PPK_ASSERT

// Template replacement for the C offsetof() macro. Unfortunately, it cannot be constexpr within
// the current rules of the language.
// Example: offset_of(&Point::x)
// See: https://gist.github.com/graphitemaster/494f21190bb2c63c5516
template<typename T1, default_initializable T2>
inline auto offset_of(T1 T2::*member) -> size_t {
	static T2 obj;
	return reinterpret_cast<size_t>(&(obj.*member)) - reinterpret_cast<size_t>(&obj);
}

// Conversion of scoped enum to the underlying type, using the unary + operator
template<enum_type T>
constexpr auto operator+(T e) {
	return static_cast<std::underlying_type_t<T>>(e);
}

// *** Deprecated ***

//TODO remove
// Clear an array, setting all bytes to 0.
#define arrayClear(arr) \
    std::memset((arr), 0, sizeof((arr)))

//TODO remove
// Copy the contents of one array into another array of the same or bigger size.
#define arrayCopy(dst, src) \
    std::memcpy((dst), (src), sizeof((dst)))

// Error-checking and type-safe malloc() wrapper. Clears all allocated memory
// to zero.
template<typename T>
auto allocate(size_t const count = 1) -> T* {
	ASSERT(count);
	auto* const result = static_cast<T*>(std::calloc(count, sizeof(T)));
	if (!result) {
		std::perror("Could not allocate memory");
		std::exit(EXIT_FAILURE);
	}
	return result;
}

}
