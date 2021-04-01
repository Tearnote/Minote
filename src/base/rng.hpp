#pragma once

#include <concepts>
#include <cmath>
#include "pcg/pcg_basic.h"
#include "base/assert.hpp"
#include "base/types.hpp"

namespace minote::base {

// PCG pseudorandom number generator
struct Rng {

	// Internal state of the RNG. The .inc (second) field must always be odd
	pcg32_random_t state = {0, 1};

	// Seed the generator with any 64bit value. The generated sequence will
	// always be the same for any given seed.
	void seed(u64 seed) {
		pcg32_srandom_r(&state, seed, 'M' * 'i' + 'n' * 'o' + 't' * 'e');
	}

	// Return a random positive integer, up to a bound (exclusive). RNG state
	// is advanced by one step.
	auto randInt(u32 bound) -> u32 {
		ASSERT(bound >= 1);
		return pcg32_boundedrand_r(&state, bound);
	}

	// Return a random floating-point value between 0.0 (inclusive) and 1.0
	// (exclusive). RNG state is advanced by one step.
	template<std::floating_point T = f32>
	auto randFloat() -> T {
		return std::ldexp(T(pcg32_random_r(&state)), -32);
	}

};

}
