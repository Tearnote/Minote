/**
 * Implementation of ease.h
 * @file
 */

#include "ease.h"

#include <assert.h>
#include "aheasing/easing.h"
#include "util.h"

/// Map of ::EaseType values to matching functions
static AHEasingFunction easeFunctions[EaseSize] = {
	null,
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

float easeApply(Ease *e)
{
	assert(e);
	nsec time = getTime();
	if (e->start >= time)
		return e->from;
	if (e->start + e->duration <= time)
		return e->to;

	nsec elapsed = time - e->start;
	float progress = (double)elapsed / (double) e->duration;
	progress = easeFunctions[e->type](progress);

	float span = e->to - e->from;
	return e->from + span * progress;
}

void easeRestart(Ease *e)
{
	assert(e);
	e->start = getTime();
}
