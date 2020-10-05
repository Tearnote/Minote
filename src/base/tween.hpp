/**
 * Smooth transitions between floating-point values
 * @file
 */

#pragma once

#include "ease.hpp"
#include "util.hpp"
#include "../time.hpp"

namespace minote {

/**
 * Description of a tween instance. most of the fields need to be filled in
 * manually before use; designated initializer syntax is convenient for this.
 */
template<FloatingType T = float>
struct Tween {

	using Type = T;

	Type from{0.0f}; ///< initial value
	Type to{1.0f}; ///< final value
	nsec start{0}; ///< time of starting the tween
	nsec duration{secToNsec(1)}; ///< time the tween will take to finish
	EasingFunction<Type> type{linearInterpolation}; ///< easing function to use during the tween

	/**
	 * Convenience function to replay a tween from the current moment.
	 */
	void restart() { start = getTime(); }

	/**
	 * Calculate the current value of the tween. The return value will
	 * be clamped if it is outside of the specified time range.
	 * @return Current tween result
	 */
	auto apply() const -> Type { return applyAt(getTime()); }

	/**
	 * Calculate the value of the tween for a specified moment in time.
	 * @param time Timestamp to calculate for
	 * @return Tween result at specified moment
	 */
	constexpr auto applyAt(nsec time) const -> Type;

};

template<FloatingType T>
constexpr auto Tween<T>::applyAt(nsec time) const -> Type
{
	if (start >= time)
		return from;
	if (start + duration <= time)
		return to;

	const nsec elapsed = time - start;
	const Type progress = type(static_cast<Type>(elapsed) / duration);

	const Type span = to - from;
	return from + span * progress;
}

}
