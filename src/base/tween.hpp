// Minote - base/tween.hpp
// Smooth transitions between floating-point values

#pragma once

#include "base/ease.hpp"
#include "base/util.hpp"
#include "base/time.hpp"
#include "sys/window.hpp"

namespace minote {

// Description of a tween instance. most of the fields need to be filled in
// manually before use; designated initializer syntax is convenient for this.
template<FloatingPoint T = float>
struct Tween {

	using Type = T;

	// Initial value
	Type from = 0.0f;

	// Final value
	Type to = 1.0f;

	// Time of starting the tween
	nsec start = 0;

	// Time the tween will take to finish
	nsec duration = seconds(1);

	// Easing function to use during the tween
	EasingFunction<Type> type = linearInterpolation;

	// Convenience function to replay a tween from the current moment.
	void restart() { start = Window::getTime(); }

	// Calculate the current value of the tween. The return value will
	// be clamped if it is outside of the specified time range.
	auto apply() const -> Type { return applyAt(Window::getTime()); }

	// Calculate the value of the tween for a specified moment in time.
	constexpr auto applyAt(nsec time) const -> Type;

};

template<FloatingPoint T>
constexpr auto Tween<T>::applyAt(nsec const time) const -> Type
{
	if (start >= time) return from;
	if (start + duration <= time) return to;

	nsec const elapsed = time - start;
	Type const progress = type(static_cast<Type>(elapsed) / duration);

	Type const span = to - from;
	return from + span * progress;
}

}
