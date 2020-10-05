/**
 * Useful objects, functions and constants to compliment the standard library
 * @file
 */

#pragma once

#include <type_traits>
#include <cassert>
#include <cstddef>
#include <cstdlib> // Provide free()
#include <cstring>
#include <cstdio>
#include <cmath>
#include "pcg/pcg_basic.h"

namespace minote {

template<typename T>
concept EnumType = std::is_enum_v<T>;

template<typename T>
concept FloatingType = std::is_floating_point_v<T>;

template<typename T>
concept IntegralType = std::is_integral_v<T>;

template<typename T>
concept Arithmetic = std::is_arithmetic_v<T>;

/// Conversion of scoped enum to the underlying type, using the unary + operator
template<EnumType T>
constexpr auto operator+(T e) {
	return static_cast<std::underlying_type_t<T>>(e);
}

/**
 * A more correct replacement for pi.
 * @see https://tauday.com/
 */
template<FloatingType T>
constexpr T Tau_v{6.283185307179586476925286766559005768L};

constexpr double Tau{Tau_v<double>};

 /**
  * Convert degrees to radians.
  * @param angle Angle in degrees
  * @return Angle in radians
  */
template<FloatingType T>
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
template<IntegralType T>
constexpr auto mod(T num, T div)
{
	return num % div + (num % div < 0) * div;
}

/// Encapsulation of a PCG pseudorandom number generator
struct Rng {

	/// Internal state of the RNG. The .inc field must always be odd.
	pcg32_random_t state{0, 1};

	/**
	 * Seed the generator with a given value. The generated sequence will always
	 * be the same for any given seed.
	 * @param seed Any 64-bit integer to be used as RNG seed
	 */
	void seed(uint64_t seed)
	{
		pcg32_srandom_r(&state, seed, 'M'*'i'+'n'*'o'+'t'*'e');
	}

	/**
	 * Return a random positive integer, up to a bound (exclusive). RNG state
     * is advanced by one step.
	 * @param bound The return value will be smaller than this argument
	 * @return A random integer
	 */
	auto randInt(uint32_t bound) -> uint32_t
	{
		assert(bound >= 1);
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

////////////////////////////////////////////////////////////////////////////////

/**
 * Return the number of elements of an array. It must not be decayed to
 * a pointer, or this information is lost.
 * @param x Array argument
 */
#define countof(x) \
    ((sizeof(x)/sizeof(0[x])) / ((std::size_t)(!(sizeof(x) % sizeof(0[x])))))

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
	assert(count);
	auto* const result{static_cast<T*>(std::calloc(count, sizeof(T)))};
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
	assert(newCount);
	auto* const result{static_cast<T*>(std::realloc(buffer, sizeof(T) * newCount))};
	if (!result) {
		std::perror("Could not allocate memory");
		std::exit(EXIT_FAILURE);
	}
	buffer = result;
}

}
