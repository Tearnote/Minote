// Minote - ease.c

#include "ease.h"

#include <string.h>
#include <stdbool.h>

#include "AHEasing/easing.h"

#include "util/timer.h"
#include "types/array.h"

struct ease {
	float *target;
	float from;
	float to;
	nsec start;
	nsec length;
	enum easeType type;
};

pdarray *eases;

static AHEasingFunction easeFunc[EaseSize] = {
	NULL,
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

void initEase(void)
{
	eases = createPdarray(sizeof(struct ease));
}

void cleanupEase(void)
{
	destroyPdarray(eases);
	eases = NULL;
}

void updateEase(void)
{
	for (int i = 0; i < eases->count; i++) {
		struct ease *ease = getPdarrayItem(eases, i);
		// Ease is inactive
		if (!isPdarrayItemAlive(eases, i))
			continue;

		// Ease has not started yet (shouldn't happen)
		nsec time = getTime();
		if (time < ease->start)
			continue;

		// Ease just finished
		if (time >= ease->start + ease->length) {
			*ease->target = ease->to;
			killPdarrayItem(eases, i);
			continue;
		}

		// Ease is in progress
		nsec elapsed = time - ease->start;
		float progress = (double)elapsed / (double)ease->length;
		progress = easeFunc[ease->type](progress);

		float span = ease->to - ease->from;
		*ease->target = ease->from + span * progress;
	}
}

void
addEase(float *target, float from, float to, nsec length, enum easeType type)
{
	struct ease *newEase = producePdarrayItem(eases);
	newEase->target = target;
	newEase->from = from;
	newEase->to = to;
	newEase->start = getTime();
	newEase->length = length;
	newEase->type = type;
}