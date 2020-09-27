/**
 * Implementation of tween.h
 * @file
 */

#include "tween.hpp"

#include <assert.h>
#include "aheasing/easing.h"
#include "util.hpp"

/// Map of ::EaseType values to matching functions
static AHEasingFunction easeFunctions[EaseSize] = {
	nullptr,
	LinearInterpolation,
	QuadraticEaseIn, QuadraticEaseOut, QuadraticEaseInOut,
	CubicEaseIn, CubicEaseOut, CubicEaseInOut,
	QuarticEaseIn, QuadraticEaseOut, QuadraticEaseInOut,
	QuinticEaseIn, QuinticEaseOut, QuinticEaseInOut,
	SineEaseIn, SineEaseOut, SineEaseInOut,
	CircularEaseIn, CircularEaseOut, CircularEaseInOut,
	ExponentialEaseIn, ExponentialEaseOut, ExponentialEaseInOut,
	ElasticEaseIn, ElasticEaseOut, ElasticEaseInOut,
	BackEaseIn, BackEaseOut, BackEaseInOut,
	BounceEaseIn, BounceEaseOut, BounceEaseInOut
};

float tweenApply(Tween* t)
{
	assert(t);
	nsec time = getTime();
	if (t->start >= time)
		return t->from;
	if (t->start + t->duration <= time)
		return t->to;

	nsec elapsed = time - t->start;
	float progress = (double)elapsed / (double) t->duration;
	progress = easeFunctions[t->type](progress);

	float span = t->to - t->from;
	return t->from + span * progress;
}

void tweenRestart(Tween* t)
{
	assert(t);
	t->start = getTime();
}
