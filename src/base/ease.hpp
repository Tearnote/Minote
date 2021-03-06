// Minote - base/ease.hpp
// Easing functions, adapted from AHEasing project to constexpr C++
// See: https://github.com/warrenm/AHEasing, used under WTFPL license

#pragma once

#include "base/concept.hpp"
#include "base/math.hpp"

namespace minote {

// Type matching all of the below functions
template<floating_point T>
using EasingFunction = auto (*)(T) -> T;

// Modeled after the line y = x
template<floating_point T>
[[maybe_unused]]
constexpr auto linearInterpolation(T const p) -> T {
	return p;
}

// Modeled after the parabola y = x^2
template<floating_point T>
[[maybe_unused]]
constexpr auto quadraticEaseIn(T const p) -> T {
	return p * p;
}

// Modeled after the parabola y = -x^2 + 2x
template<floating_point T>
[[maybe_unused]]
constexpr auto quadraticEaseOut(T const p) -> T {
	return -(p * (p - 2));
}

// Modeled after the piecewise quadratic
// y = (1/2)((2x)^2)             ; [0, 0.5)
// y = -(1/2)((2x-1)*(2x-3) - 1) ; [0.5, 1]
template<floating_point T>
[[maybe_unused]]
constexpr auto quadraticEaseInOut(T const p) -> T {
	if (p < 0.5)
		return 2 * p * p;
	else
		return (-2 * p * p) + (4 * p) - 1;
}

// Modeled after the cubic y = x^3
template<floating_point T>
[[maybe_unused]]
constexpr auto cubicEaseIn(T const p) -> T {
	return p * p * p;
}

// Modeled after the cubic y = (x - 1)^3 + 1
template<floating_point T>
[[maybe_unused]]
constexpr auto cubicEaseOut(T const p) -> T {
	T f = p - 1;
	return f * f * f + 1;
}

// Modeled after the piecewise cubic
// y = (1/2)((2x)^3)       ; [0, 0.5)
// y = (1/2)((2x-2)^3 + 2) ; [0.5, 1]
template<floating_point T>
[[maybe_unused]]
constexpr auto cubicEaseInOut(T const p) -> T {
	if (p < 0.5) {
		return 4 * p * p * p;
	} else {
		T f = (2 * p) - 2;
		return 0.5 * f * f * f + 1;
	}
}

// Modeled after the quartic x^4
template<floating_point T>
[[maybe_unused]]
constexpr auto quarticEaseIn(T const p) -> T {
	return p * p * p * p;
}

// Modeled after the quartic y = 1 - (x - 1)^4
template<floating_point T>
[[maybe_unused]]
constexpr auto quarticEaseOut(T const p) -> T {
	T f = p - 1;
	return f * f * f * (1 - p) + 1;
}

// Modeled after the piecewise quartic
// y = (1/2)((2x)^4)        ; [0, 0.5)
// y = -(1/2)((2x-2)^4 - 2) ; [0.5, 1]
template<floating_point T>
[[maybe_unused]]
constexpr auto quarticEaseInOut(T const p) -> T {
	if (p < 0.5) {
		return 8 * p * p * p * p;
	} else {
		T f = p - 1;
		return -8 * f * f * f * f + 1;
	}
}

// Modeled after the quintic y = x^5
template<floating_point T>
[[maybe_unused]]
constexpr auto quinticEaseIn(T const p) -> T {
	return p * p * p * p * p;
}

// Modeled after the quintic y = (x - 1)^5 + 1
template<floating_point T>
[[maybe_unused]]
constexpr auto quinticEaseOut(T const p) -> T {
	T f = p - 1;
	return f * f * f * f * f + 1;
}

// Modeled after the piecewise quintic
// y = (1/2)((2x)^5)       ; [0, 0.5)
// y = (1/2)((2x-2)^5 + 2) ; [0.5, 1]
template<floating_point T>
[[maybe_unused]]
constexpr auto quinticEaseInOut(T const p) -> T {
	if (p < 0.5) {
		return 16 * p * p * p * p * p;
	} else {
		T f = (2 * p) - 2;
		return 0.5 * f * f * f * f * f + 1;
	}
}

// Modeled after quarter-cycle of sine wave
template<floating_point T>
[[maybe_unused]]
constexpr auto sineEaseIn(T const p) -> T {
	return sin((p - 1) * Tau_v<T> / 4) + 1;
}

// Modeled after quarter-cycle of sine wave (different phase)
template<floating_point T>
[[maybe_unused]]
constexpr auto sineEaseOut(T const p) -> T {
	return sin(p * Tau_v<T> / 4);
}

// Modeled after half sine wave
template<floating_point T>
[[maybe_unused]]
constexpr auto sineEaseInOut(T const p) -> T {
	return 0.5 * (1 - cos(p * Tau_v<T> / 2));
}

// Modeled after shifted quadrant IV of unit circle
template<floating_point T>
[[maybe_unused]]
constexpr auto circularEaseIn(T const p) -> T {
	return 1 - sqrt(1 - (p * p));
}

// Modeled after shifted quadrant II of unit circle
template<floating_point T>
[[maybe_unused]]
constexpr auto circularEaseOut(T const p) -> T {
	return sqrt((2 - p) * p);
}

// Modeled after the piecewise circular function
// y = (1/2)(1 - sqrt(1 - 4x^2))           ; [0, 0.5)
// y = (1/2)(sqrt(-(2x - 3)*(2x - 1)) + 1) ; [0.5, 1]
template<floating_point T>
[[maybe_unused]]
constexpr auto circularEaseInOut(T const p) -> T {
	if (p < 0.5)
		return 0.5 * (1 - sqrt(1 - 4 * (p * p)));
	else
		return 0.5 * (sqrt(-((2 * p) - 3) * ((2 * p) - 1)) + 1);
}

// Modeled after the exponential function y = 2^(10(x - 1))
template<floating_point T>
[[maybe_unused]]
constexpr auto exponentialEaseIn(T const p) -> T {
	return (p == 0.0)? p : pow(2, 10 * (p - 1));
}

// Modeled after the exponential function y = -2^(-10x) + 1
template<floating_point T>
[[maybe_unused]]
constexpr auto exponentialEaseOut(T const p) -> T {
	return (p == 1.0)? p : 1 - pow(2, -10 * p);
}

// Modeled after the piecewise exponential
// y = (1/2)2^(10(2x - 1))         ; [0,0.5)
// y = -(1/2)*2^(-10(2x - 1))) + 1 ; [0.5,1]
template<floating_point T>
[[maybe_unused]]
constexpr auto exponentialEaseInOut(T const p) -> T {
	if (p == 0.0 || p == 1.0)
		return p;

	if (p < 0.5)
		return 0.5 * pow(2, (20 * p) - 10);
	else
		return -0.5 * pow(2, (-20 * p) + 10) + 1;
}

// Modeled after the damped sine wave y = sin(13pi/2*x)*pow(2, 10 * (x - 1))
template<floating_point T>
[[maybe_unused]]
constexpr auto elasticEaseIn(T const p) -> T {
	return sin(13 * (Tau_v<T> / 4) * p) * pow(2, 10 * (p - 1));
}

// Modeled after the damped sine wave y = sin(-13pi/2*(x + 1))*pow(2, -10x) + 1
template<floating_point T>
[[maybe_unused]]
constexpr auto elasticEaseOut(T const p) -> T {
	return sin(-13 * (Tau_v<T> / 4) * (p + 1)) * pow(2, -10 * p) + 1;
}

// Modeled after the piecewise exponentially-damped sine wave:
// y = (1/2)*sin(13pi/2*(2*x))*pow(2, 10 * ((2*x) - 1))      ; [0,0.5)
// y = (1/2)*(sin(-13pi/2*((2x-1)+1))*pow(2,-10(2*x-1)) + 2) ; [0.5, 1]
template<floating_point T>
[[maybe_unused]]
constexpr auto elasticEaseInOut(T const p) -> T {
	if (p < 0.5) {
		return 0.5 * sin(13 * (Tau_v<T> / 4) * (2 * p)) * pow(2, 10 * ((2 * p) - 1));
	} else {
		return 0.5 * (sin(-13 * (Tau_v<T> / 4) * ((2 * p - 1) + 1)) *
			pow(2, -10 * (2 * p - 1)) + 2);
	}
}

// Modeled after the overshooting cubic y = x^3-x*sin(x*pi)
template<floating_point T>
[[maybe_unused]]
constexpr auto eackEaseIn(T const p) -> T {
	return p * p * p - p * sin(p * Tau_v<T> / 2);
}

// Modeled after overshooting cubic y = 1-((1-x)^3-(1-x)*sin((1-x)*pi))
template<floating_point T>
[[maybe_unused]]
constexpr auto backEaseOut(T const p) -> T {
	T f = 1 - p;
	return 1 - (f * f * f - f * sin(f * Tau_v<T> / 2));
}

// Modeled after the piecewise overshooting cubic function:
// y = (1/2)*((2x)^3-(2x)*sin(2*x*pi))           ; [0, 0.5)
// y = (1/2)*(1-((1-x)^3-(1-x)*sin((1-x)*pi))+1) ; [0.5, 1]
template<floating_point T>
[[maybe_unused]]
constexpr auto backEaseInOut(T const p) -> T {
	if (p < 0.5) {
		T f = 2 * p;
		return 0.5 * (f * f * f - f * sin(f * Tau_v<T> / 2));
	} else {
		T f = (1 - (2 * p - 1));
		return 0.5 * (1 - (f * f * f - f * sin(f * Tau_v<T> / 2))) + 0.5;
	}
}

template<floating_point T>
[[maybe_unused]]
constexpr auto bounceEaseIn(T const p) -> T {
	return 1 - bounceEaseOut(1 - p);
}

template<floating_point T>
[[maybe_unused]]
constexpr auto bounceEaseOut(T const p) -> T {
	if (p < 4 / 11.0)
		return (121 * p * p) / 16.0;
	else if (p < 8 / 11.0)
		return (363 / 40.0 * p * p) - (99 / 10.0 * p) + 17 / 5.0;
	else if (p < 9 / 10.0)
		return (4356 / 361.0 * p * p) - (35442 / 1805.0 * p) + 16061 / 1805.0;
	else
		return (54 / 5.0 * p * p) - (513 / 25.0 * p) + 268 / 25.0;
}

template<floating_point T>
[[maybe_unused]]
constexpr auto bounceEaseInOut(T const p) -> T {
	if (p < 0.5)
		return 0.5 * bounceEaseIn(p * 2);
	else
		return 0.5 * bounceEaseOut(p * 2 - 1) + 0.5;
}

}
