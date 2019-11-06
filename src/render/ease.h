// Minote - ease.h
// Change a floating point variable smoothly over time

#ifndef EASE_H
#define EASE_H

#include "util/timer.h"

enum easeType {
	EaseNone,
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
	EaseSize
};

void initEase(void);
void cleanupEase(void);

void updateEase(void);

void
addEase(float *target, float from, float to, nsec length, enum easeType type);

#endif //EASE_H
