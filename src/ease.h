/**
 * Smooth transitions between floating-point values
 * @file
 */

#ifndef MINOTE_EASE_H
#define MINOTE_EASE_H

#include "time.h"

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
 * Description of an easing instance. To use this with below functions,
 * you need to fill in most of the fields manually. However, helper functions
 * exist to reuse the same instance repeatedly.
 */
typedef struct Ease {
	float from; ///< initial value
	float to; ///< final value
	nsec start; ///< time of starting the ease
	nsec length; ///< length of time that the ease will take to finish
	EaseType type; ///< easing function to use during the ease
} Ease;

/**
 * Calculate the current value of an ::Ease. It is safe to call this outside
 * of the specified time range, both before and after - the value will
 * be clamped.
 * @param e The ::Ease data
 * @return Calculated value
 */
float easeApply(Ease *e);

/**
 * Move an ::Ease's starting position to current time, replaying a configured
 * instance.
 * @param e The ::Ease data
 */
void easeRestart(Ease *e);

#endif //MINOTE_EASE_H
