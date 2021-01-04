#pragma once

#include <concepts>
#include <numbers>
#include <cmath>

namespace minote::base {

// Type matching all of the below functions
template<std::floating_point T>
using EasingFunction = auto (*)(T) -> T;

// Modeled after the line y = x
template<std::floating_point T>
constexpr auto linearInterpolation(T p) -> T {
	return p;
}

// Modeled after the parabola y = x^2
template<std::floating_point T>
constexpr auto quadraticEaseIn(T p) -> T {
	return p * p;
}

// Modeled after the parabola y = -x^2 + 2x
template<std::floating_point T>
constexpr auto quadraticEaseOut(T p) -> T {
	return -(p * (p - 2));
}

// Modeled after the piecewise quadratic
// y = (1/2)((2x)^2)             ; [0, 0.5)
// y = -(1/2)((2x-1)*(2x-3) - 1) ; [0.5, 1]
template<std::floating_point T>
constexpr auto quadraticEaseInOut(T p) -> T {
	if (p < 0.5)
		return 2 * p * p;
	else
		return (-2 * p * p) + (4 * p) - 1;
}

// Modeled after the cubic y = x^3
template<std::floating_point T>
constexpr auto cubicEaseIn(T p) -> T {
	return p * p * p;
}

// Modeled after the cubic y = (x - 1)^3 + 1
template<std::floating_point T>
constexpr auto cubicEaseOut(T p) -> T {
	T f = p - 1;
	return f * f * f + 1;
}

// Modeled after the piecewise cubic
// y = (1/2)((2x)^3)       ; [0, 0.5)
// y = (1/2)((2x-2)^3 + 2) ; [0.5, 1]
template<std::floating_point T>
constexpr auto cubicEaseInOut(T p) -> T {
	if (p < 0.5) {
		return 4 * p * p * p;
	} else {
		T f = (2 * p) - 2;
		return 0.5 * f * f * f + 1;
	}
}

// Modeled after the quartic x^4
template<std::floating_point T>
constexpr auto quarticEaseIn(T p) -> T {
	return p * p * p * p;
}

// Modeled after the quartic y = 1 - (x - 1)^4
template<std::floating_point T>
constexpr auto quarticEaseOut(T p) -> T {
	T f = p - 1;
	return f * f * f * (1 - p) + 1;
}

// Modeled after the piecewise quartic
// y = (1/2)((2x)^4)        ; [0, 0.5)
// y = -(1/2)((2x-2)^4 - 2) ; [0.5, 1]
template<std::floating_point T>
constexpr auto quarticEaseInOut(T p) -> T {
	if (p < 0.5) {
		return 8 * p * p * p * p;
	} else {
		T f = p - 1;
		return -8 * f * f * f * f + 1;
	}
}

// Modeled after the quintic y = x^5
template<std::floating_point T>
constexpr auto quinticEaseIn(T p) -> T {
	return p * p * p * p * p;
}

// Modeled after the quintic y = (x - 1)^5 + 1
template<std::floating_point T>
constexpr auto quinticEaseOut(T p) -> T {
	T f = p - 1;
	return f * f * f * f * f + 1;
}

// Modeled after the piecewise quintic
// y = (1/2)((2x)^5)       ; [0, 0.5)
// y = (1/2)((2x-2)^5 + 2) ; [0.5, 1]
template<std::floating_point T>
constexpr auto quinticEaseInOut(T p) -> T {
	if (p < 0.5) {
		return 16 * p * p * p * p * p;
	} else {
		T f = (2 * p) - 2;
		return 0.5 * f * f * f * f * f + 1;
	}
}

// Modeled after quarter-cycle of sine wave
template<std::floating_point T>
constexpr auto sineEaseIn(T p) -> T {
	return std::sin((p - 1) * std::pi_v<T> / 2) + 1;
}

// Modeled after quarter-cycle of sine wave (different phase)
template<std::floating_point T>
constexpr auto sineEaseOut(T p) -> T {
	return std::sin(p * std::pi_v<T> / 2);
}

// Modeled after half sine wave
template<std::floating_point T>
constexpr auto sineEaseInOut(T p) -> T {
	return 0.5 * (1 - std::cos(p * std::pi_v<T>));
}

// Modeled after shifted quadrant IV of unit circle
template<std::floating_point T>
constexpr auto circularEaseIn(T p) -> T {
	return 1 - std::sqrt(1 - (p * p));
}

// Modeled after shifted quadrant II of unit circle
template<std::floating_point T>
constexpr auto circularEaseOut(T p) -> T {
	return std::sqrt((2 - p) * p);
}

// Modeled after the piecewise circular function
// y = (1/2)(1 - sqrt(1 - 4x^2))           ; [0, 0.5)
// y = (1/2)(sqrt(-(2x - 3)*(2x - 1)) + 1) ; [0.5, 1]
template<std::floating_point T>
constexpr auto circularEaseInOut(T p) -> T {
	if (p < 0.5)
		return 0.5 * (1 - std::sqrt(1 - 4 * (p * p)));
	else
		return 0.5 * (std::sqrt(-((2 * p) - 3) * ((2 * p) - 1)) + 1);
}

// Modeled after the exponential function y = 2^(10(x - 1))
template<std::floating_point T>
constexpr auto exponentialEaseIn(T p) -> T {
	return (p == 0.0)? p : std::pow(2, 10 * (p - 1));
}

// Modeled after the exponential function y = -2^(-10x) + 1
template<std::floating_point T>
constexpr auto exponentialEaseOut(T p) -> T {
	return (p == 1.0)? p : 1 - std::pow(2, -10 * p);
}

// Modeled after the piecewise exponential
// y = (1/2)2^(10(2x - 1))         ; [0,0.5)
// y = -(1/2)*2^(-10(2x - 1))) + 1 ; [0.5,1]
template<std::floating_point T>
constexpr auto exponentialEaseInOut(T p) -> T {
	if (p == 0.0 || p == 1.0)
		return p;

	if (p < 0.5)
		return 0.5 * std::pow(2, (20 * p) - 10);
	else
		return -0.5 * std::pow(2, (-20 * p) + 10) + 1;
}

// Modeled after the damped sine wave y = sin(13pi/2*x)*pow(2, 10 * (x - 1))
template<std::floating_point T>
constexpr auto elasticEaseIn(T p) -> T {
	return std::sin(13 * (std::pi_v<T> / 2) * p) * std::pow(2, 10 * (p - 1));
}

// Modeled after the damped sine wave y = sin(-13pi/2*(x + 1))*pow(2, -10x) + 1
template<std::floating_point T>
constexpr auto elasticEaseOut(T p) -> T {
	return std::sin(-13 * (std::pi_v<T> / 2) * (p + 1)) * std::pow(2, -10 * p) + 1;
}

// Modeled after the piecewise exponentially-damped sine wave:
// y = (1/2)*sin(13pi/2*(2*x))*pow(2, 10 * ((2*x) - 1))      ; [0,0.5)
// y = (1/2)*(sin(-13pi/2*((2x-1)+1))*pow(2,-10(2*x-1)) + 2) ; [0.5, 1]
template<std::floating_point T>
constexpr auto elasticEaseInOut(T p) -> T {
	if (p < 0.5) {
		return 0.5 * std::sin(13 * (std::pi_v<T> / 2) * (2 * p)) *
			std::pow(2, 10 * ((2 * p) - 1));
	} else {
		return 0.5 * (std::sin(-13 * (std::pi_v<T> / 2) * ((2 * p - 1) + 1)) *
			std::pow(2, -10 * (2 * p - 1)) + 2);
	}
}

// Modeled after the overshooting cubic y = x^3-x*sin(x*pi)
template<std::floating_point T>
constexpr auto backEaseIn(T p) -> T {
	return p * p * p - p * std::sin(p * std::pi_v<T>);
}

// Modeled after overshooting cubic y = 1-((1-x)^3-(1-x)*sin((1-x)*pi))
template<std::floating_point T>
constexpr auto backEaseOut(T p) -> T {
	T f = 1 - p;
	return 1 - (f * f * f - f * std::sin(f * std::pi_v<T>));
}

// Modeled after the piecewise overshooting cubic function:
// y = (1/2)*((2x)^3-(2x)*sin(2*x*pi))           ; [0, 0.5)
// y = (1/2)*(1-((1-x)^3-(1-x)*sin((1-x)*pi))+1) ; [0.5, 1]
template<std::floating_point T>
constexpr auto backEaseInOut(T p) -> T {
	if (p < 0.5) {
		T f = 2 * p;
		return 0.5 * (f * f * f - f * std::sin(f * std::pi_v<T>));
	} else {
		T f = (1 - (2 * p - 1));
		return 0.5 * (1 - (f * f * f - f * std::sin(f * std::pi_v<T>))) + 0.5;
	}
}

template<std::floating_point T>
constexpr auto bounceEaseIn(T p) -> T {
	return 1 - bounceEaseOut(1 - p);
}

template<std::floating_point T>
constexpr auto bounceEaseOut(T p) -> T {
	if (p < 4 / 11.0)
		return (121 * p * p) / 16.0;
	else if (p < 8 / 11.0)
		return (363 / 40.0 * p * p) - (99 / 10.0 * p) + 17 / 5.0;
	else if (p < 9 / 10.0)
		return (4356 / 361.0 * p * p) - (35442 / 1805.0 * p) + 16061 / 1805.0;
	else
		return (54 / 5.0 * p * p) - (513 / 25.0 * p) + 268 / 25.0;
}

template<std::floating_point T>
constexpr auto bounceEaseInOut(T p) -> T {
	if (p < 0.5)
		return 0.5 * bounceEaseIn(p * 2);
	else
		return 0.5 * bounceEaseOut(p * 2 - 1) + 0.5;
}

}
