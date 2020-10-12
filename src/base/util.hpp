/**
 * Useful objects, functions and constants to compliment the standard library
 * @file
 */

#pragma once

#include <type_traits>
#include <cstddef>
#include <cstdint>
#include <cstdlib> // Provide free()
#include <cstring>
#include <cstdio>
#include <cmath>
#include "scope_guard/scope_guard.hpp"
#include "PPK_ASSERT/ppk_assert.h"
#include "pcg/pcg_basic.h"

namespace minote {

/// Make the DEFER feature from scope_guard more keyword-like
#define defer DEFER

/// Improved assert() macro from PPK_ASSERT, with printf-like formatting
#define ASSERT PPK_ASSERT

template<typename T>
concept Enum = std::is_enum_v<T>;

template<typename T>
concept FloatingPoint = std::is_floating_point_v<T>;

template<typename T>
concept Integral = std::is_integral_v<T>;

template<typename T>
concept Arithmetic = std::is_arithmetic_v<T>;

template<typename T>
concept Standard = std::is_standard_layout_v<T>;

/// Conversion of scoped enum to the underlying type, using the unary + operator
template<Enum T>
constexpr auto operator+(T e) {
	return static_cast<std::underlying_type_t<T>>(e);
}

/**
 * A more correct replacement for pi.
 * @see https://tauday.com/
 */
template<FloatingPoint T>
constexpr T Tau_v = 6.283185307179586476925286766559005768L;

constexpr auto Tau = Tau_v<double>;

 /**
  * Convert degrees to radians.
  * @param angle Angle in degrees
  * @return Angle in radians
  */
template<FloatingPoint T>
constexpr auto rad(T angle)
{
	return angle * Tau_v<T> / static_cast<T>(360.0);
}

/**
 * True modulo operation (as opposed to remainder, which is operator% in C++.)
 * @param num Value
 * @param div Modulo divisor
 * @return Result of modulo (always positive)
 */
template<Integral T>
constexpr auto mod(T num, T div)
{
	return num % div + (num % div < 0) * div;
}

/// Encapsulation of a PCG pseudorandom number generator
struct Rng {

	/// Internal state of the RNG. The .inc field must always be odd.
	pcg32_random_t state = {0, 1};

	/**
	 * Seed the generator with a given value. The generated sequence will always
	 * be the same for any given seed.
	 * @param seed Any 64-bit integer to be used as RNG seed
	 */
	void seed(std::uint64_t seed)
	{
		pcg32_srandom_r(&state, seed, 'M'*'i'+'n'*'o'+'t'*'e');
	}

	/**
	 * Return a random positive integer, up to a bound (exclusive). RNG state
     * is advanced by one step.
	 * @param bound The return value will be smaller than this argument
	 * @return A random integer
	 */
	auto randInt(std::uint32_t bound) -> std::uint32_t
	{
		ASSERT(bound >= 1);
		return pcg32_boundedrand_r(&state, bound);
	}

	/**
	 * Return a random floating-point value between 0.0 (inclusive) and 1.0
     * (exclusive). RNG state is advanced by one step.
	 * @return A random floating-point number
	 */
	auto randFloat()
	{
		return std::ldexp(pcg32_random_r(&state), -32);
	}

};

/**
 * Turn a string into "NULL" if it is nullptr.
 * @param str String to test
 * @return Itself, or "NULL" literal
 */
inline auto stringOrNull(const char* str) -> const char*
{
	if (str)
		return str;
	else
		return "NULL";
}

////////////////////////////////////////////////////////////////////////////////

/**
 * Clear an array, setting all bytes to 0.
 * @param arr Array argument
 */
#define arrayClear(arr) \
    std::memset((arr), 0, sizeof((arr)))

/**
 * Copy the contents of one array into another array of the same or bigger size.
 * @param dst Destination of the copy
 * @param src Source of the copy
 */
#define arrayCopy(dst, src) \
    std::memcpy((dst), (src), sizeof((dst)))

/**
 * Error-checking and type-safe malloc() wrapper. Clears all allocated memory
 * to 0.
 * @param count Number of elements to allocate memory for
 * @return Pointer to allocated memory
 */
template<typename T>
auto allocate(const std::size_t count = 1) -> T*
{
	ASSERT(count);
	auto* const result = static_cast<T*>(std::calloc(count, sizeof(T)));
	if (!result) {
		std::perror("Could not allocate memory");
		std::exit(EXIT_FAILURE);
	}
	return result;
}

/**
 * Error-checking and type-safe realloc() wrapper. The buffer changes size
 * in-place, and the pointer might change in the process. If the buffer
 * is grown, the additional space is not cleared to 0 - this should be done
 * manually to keep all data in a defined state.
 * @param[inout] buffer Pointer to previously allocated memory
 * @param newSize New number of elements to allocate memory for
 */
template<typename T>
void reallocate(T*& buffer, const std::size_t newCount)
{
	ASSERT(newCount);
	auto* const result = static_cast<T*>(std::realloc(buffer, sizeof(T) * newCount));
	if (!result) {
		std::perror("Could not allocate memory");
		std::exit(EXIT_FAILURE);
	}
	buffer = result;
}

}
