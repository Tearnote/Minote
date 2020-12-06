// Minote - base/rng.hpp
// Simple random number generator using PCG algorithm

#pragma once

#include "pcg/pcg_basic.h"
#include "base/util.hpp"

namespace minote {

// PCG pseudorandom number generator
struct Rng {

	// Internal state of the RNG. The .inc (second) field must always be odd
	pcg32_random_t state = {0, 1};

	// Seed the generator with any 64bit value. The generated sequence will
	// always be the same for any given seed.
	void seed(u64 const seed) {
		pcg32_srandom_r(&state, seed, 'M' * 'i' + 'n' * 'o' + 't' * 'e');
	}

	// Return a random positive integer, up to a bound (exclusive). RNG state
	// is advanced by one step.
	auto randInt(u32 const bound) -> u32 {
		DASSERT(bound >= 1);
		return pcg32_boundedrand_r(&state, bound);
	}

	// Return a random floating-point value between 0.0 (inclusive) and 1.0
	// (exclusive). RNG state is advanced by one step.
	template<floating_point T = float>
	auto randFloat() -> T {
		return ldexp(static_cast<T>(pcg32_random_r(&state)), -32);
	}

};

}
