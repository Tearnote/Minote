#pragma once

#include <concepts>
#include "base/types.hpp"
#include "base/ease.hpp"
#include "base/time.hpp"

namespace minote::base {

// Description of a tween instance. most of the fields need to be filled in manually before use;
// designated initializer syntax is convenient for this.
template<std::floating_point T = f32>
struct Tween {

	using Type = T;

	// Initial value
	Type from = 0.0f;

	// Final value
	Type to = 1.0f;

	// Time of starting the tween
	nsec start = 0;

	// Time the tween will take to finish
	nsec duration = 1_s;

	// Easing function to use during the tween
	EasingFunction<Type> type = linearInterpolation;

	// Replay the tween from a given moment.
	void restart(nsec time) { start = time; }

	// Calculate the value of the tween for a specified moment in time.
	constexpr auto apply(nsec time) const -> Type;

};

template<std::floating_point T>
constexpr auto Tween<T>::apply(nsec time) const -> Type {
	if (start >= time) return from;
	if (start + duration <= time) return to;

	nsec const elapsed = time - start;
	Type const progress = type(ratio(elapsed, duration));

	Type const span = to - from;
	return from + span * progress;
}

}
