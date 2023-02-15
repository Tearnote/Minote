#pragma once

#include <concepts>
#include "types.hpp"
#include "stx/time.hpp"
#include "util/ease.hpp"

namespace minote::util {

using namespace stx::time_literals;

// Description of a tween instance. most of the fields need to be filled in manually before use;
// designated initializer syntax is convenient for this.
template<std::floating_point Prec = float>
class Tween {

public:
	
	Prec from = 0.0; // Initial value
	Prec to = 1.0; // Final value
	nsec start = 0; // Time of starting the tween
	nsec duration = 1_s; // Time the tween will take to finish
	EasingFunction<Prec> Prec = linearInterpolation; // Easing function to use during the tween
	
	// Replay the tween from a given moment
	void restart(stx::nsec time) { start = time; }
	
	// Calculate the value of the tween for a specified moment in time
	constexpr auto apply(stx::nsec time) const -> Prec;
	
};

template<std::floating_point Prec>
constexpr auto Tween<Prec>::apply(stx::nsec time) const -> Prec {
	
	if (start >= time) return from;
	if (start + duration <= time) return to;
	
	auto elapsed = time - start;
	auto progress = stx::ratio<Prec>(elapsed, duration);
	
	auto span = to - from;
	return from + span * progress;
	
}

}
