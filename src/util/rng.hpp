#pragma once

#include <cmath>
#include "pcg_basic.h"
#include "util/concepts.hpp"
#include "util/verify.hpp"
#include "util/types.hpp"

namespace minote {

// PCG pseudorandom number generator
struct Rng {
	
	// Seed the generator with any 64bit value. The generated sequence will
	// always be the same for any given seed.
	void seed(u64 seed) { pcg32_srandom_r(&m_state, seed, InitSeq); }
	
	// Return a random positive integer, up to a bound (exclusive). RNG state
	// is advanced by one step.
	auto randInt(u32 bound) -> u32 {
		ASSUME(bound >= 1);
		return pcg32_boundedrand_r(&m_state, bound);
	}
	
	// Return a random floating-point value between 0.0 (inclusive) and 1.0
	// (exclusive). RNG state is advanced by one step.
	template<floating_point T = f32>
	auto randFloat() -> T {
		return std::ldexp(T(pcg32_random_r(&m_state)), -32);
	}
	
private:
	
	static constexpr auto InitSeq = u64('M' * 'i' + 'n' * 'o' + 't' * 'e');
	
	// Internal state of the RNG. The .inc (second) field must always be odd
	pcg32_random_t m_state = {0, 1};
	
};

}
