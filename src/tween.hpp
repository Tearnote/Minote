/**
 * Smooth transitions between floating-point values
 * @file
 */

#ifndef MINOTE_TWEEN_H
#define MINOTE_TWEEN_H

#include "time.hpp"

/**
 * The various kinds of easing functions.
 * @see https://easings.net/en
 */
typedef enum EaseType {
	EaseNone, ///< zero value
	EaseLinear,
	EaseInQuadratic, EaseOutQuadratic, EaseInOutQuadratic,
	EaseInCubic, EaseOutCubic, EaseInOutCubic,
	EaseInQuartic, EaseOutQuartic, EaseInOutQuartic,
	EaseInQuintic, EaseOutQuintic, EaseInOutQuintic,
	EaseInSine, EaseOutSine, EaseInOutSine,
	EaseInCircular, EaseOutCircular, EaseInOutCircular,
	EaseInExponential, EaseOutExponential, EaseInOutExponential,
	EaseInElastic, EaseOutElastic, EaseInOutElastic,
	EaseInBack, EaseOutBack, EaseInOutBack,
	EaseInBounce, EaseOutBounce, EaseInOutBounce,
	EaseSize ///< terminator
} EaseType;

/**
 * Description of a tween instance. To use this with below functions,
 * you need to fill in most of the fields manually. However, helper functions
 * exist to reuse the same instance repeatedly.
 */
typedef struct Tween {
	float from; ///< initial value
	float to; ///< final value
	nsec start; ///< time of starting the tween
	nsec duration; ///< length of time that the tween will take to finish
	EaseType type; ///< easing function to use during the tween
} Tween;

/**
 * Calculate the current value of a ::Tween. It is safe to call this outside
 * of the specified time range, both before and after - the value will
 * be clamped.
 * @param t The ::Tween data
 * @return Calculated value
 */
float tweenApply(Tween* t);

/**
 * Move a ::Tween's starting position to current time, replaying a configured
 * instance.
 * @param t The ::Tween data
 */
void tweenRestart(Tween* t);

#endif //MINOTE_TWEEN_H
