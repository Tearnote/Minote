// Minote - ease.c

#include "ease.h"

#include <stdbool.h>

#include "AHEasing/easing.h"

#include "timer.h"
#include "queue.h"

struct ease {
	float *target;
	float from;
	float to;
	nsec start;
	nsec length;
	enum easeType type;
	bool finished;
};

queue *eases;

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

static struct ease *getNewEase(void)
{
	struct ease *result = NULL;
	for (int i = 0; i < eases->count; i++) {
		result = getQueueItem(eases, i);
		if (result->finished)
			return result;
	}
	return produceQueueItem(eases);
}

void initEase(void)
{
	eases = createQueue(sizeof(struct ease));
}

void cleanupEase(void)
{
	destroyQueue(eases);
	eases = NULL;
}

void updateEase(void)
{
	for (int i = 0; i < eases->count; i++) {
		struct ease *ease = getQueueItem(eases, i);
		// Ease is inactive
		if (ease->finished)
			continue;

		// Ease has not started yet (shouldn't happen)
		nsec time = getTime();
		if (time < ease->start)
			continue;

		// Ease just finished
		if (time >= ease->start + ease->length) {
			*ease->target = ease->to;
			ease->finished = true;
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
	struct ease *newEase = getNewEase();
	newEase->target = target;
	newEase->from = from;
	newEase->to = to;
	newEase->start = getTime();
	newEase->length = length;
	newEase->type = type;
	newEase->finished = false;
}