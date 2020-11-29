// Minote - base/util.hpp
// Useful objects, functions and constants to compliment the standard library

#pragma once

#include <string_view>
#include <type_traits>
#include <filesystem>
#include <fstream>
#include <cstdint>
#include <cstdlib> // Provide free()
#include <cstring>
#include <cstdio>
#include "scope_guard/scope_guard.hpp"
#include "PPK_ASSERT/ppk_assert.h"
#include "pcg/pcg_basic.h"

namespace minote {

// *** Core concepts ***

template<typename T>
concept Enum = std::is_enum_v<T>;

template<typename T>
concept FloatingPoint = std::is_floating_point_v<T>;

template<typename T>
concept Integral = std::is_integral_v<T>;

template<typename T>
concept Arithmetic = std::is_arithmetic_v<T>;

template<typename T>
concept TriviallyCopyable = std::is_trivially_copyable_v<T>;

template<typename T>
concept ZeroArgConstructible = std::is_constructible_v<T>;

// *** Language features ***

// Make the DEFER feature from scope_guard more keyword-like
#define defer DEFER

// Improved assert() macro from PPK_ASSERT, with printf-like formatting
#define ASSERT PPK_ASSERT

// Template replacement for the C offsetof() macro. Unfortunately, it cannot
// be constexpr within the current rules of the language.
// Example: offset_of(&Point::x)
// See: https://gist.github.com/graphitemaster/494f21190bb2c63c5516
template<typename T1, ZeroArgConstructible T2>
inline auto offset_of(T1 T2::*member) -> size_t {
	static T2 obj;
	return reinterpret_cast<size_t>(&(obj.*member)) - reinterpret_cast<size_t>(&obj);
}

// Conversion of scoped enum to the underlying type, using the unary + operator
template<Enum T>
constexpr auto operator+(T e) {
	return static_cast<std::underlying_type_t<T>>(e);
}

// *** Standard library imports ***

// Used every time the size of a container is stored. Should be a keyword really
using std::size_t;

// Enable usage of standard literals
using namespace std::literals;

// Safer, UTF-8 only replacement to const char*
using std::string_view;

// A distinct type for file paths
using path = std::filesystem::path;

// *** Various utilities ***

// PCG pseudorandom number generator
struct Rng {

	// Internal state of the RNG. The .inc field must always be odd
	pcg32_random_t state = {0, 1};

	// Seed the generator with any 64bit value. The generated sequence will
	// always be the same for any given seed.
	void seed(std::uint64_t const seed)
	{
		pcg32_srandom_r(&state, seed, 'M' * 'i' + 'n' * 'o' + 't' * 'e');
	}

	// Return a random positive integer, up to a bound (exclusive). RNG state
    // is advanced by one step.
	auto randInt(std::uint32_t const bound) -> std::uint32_t
	{
		ASSERT(bound >= 1);
		return pcg32_boundedrand_r(&state, bound);
	}

	// Return a random floating-point value between 0.0 (inclusive) and 1.0
    // (exclusive). RNG state is advanced by one step.
	template<FloatingPoint T = float>
	auto randFloat() -> T
	{
		return ldexp(static_cast<T>(pcg32_random_r(&state)), -32);
	}

};

// Return the "NULL" string literal if passed argument is nullptr, otherwise
// return the argument.
inline auto stringOrNull(char const* const str) -> char const*
{
	if (str) return str;
	else return "NULL";
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
auto allocate(size_t const count = 1) -> T*
{
	ASSERT(count);
	auto* const result = static_cast<T*>(std::calloc(count, sizeof(T)));
	if (!result) {
		std::perror("Could not allocate memory");
		std::exit(EXIT_FAILURE);
	}
	return result;
}

}
