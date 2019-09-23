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

static float applyEase(float val, enum easeType type)
{
	switch (type) {
	case EaseInQuadratic:
		return QuadraticEaseIn(val);
	case EaseOutQuadratic:
		return QuadraticEaseOut(val);
	case EaseInOutQuadratic:
		return QuadraticEaseInOut(val);
	case EaseInCubic:
		return CubicEaseIn(val);
	case EaseOutCubic:
		return CubicEaseOut(val);
	case EaseInOutCubic:
		return CubicEaseInOut(val);
	case EaseInQuartic:
		return QuarticEaseIn(val);
	case EaseOutQuartic:
		return QuarticEaseOut(val);
	case EaseInOutQuartic:
		return QuarticEaseInOut(val);
	case EaseInQuintic:
		return QuinticEaseIn(val);
	case EaseOutQuintic:
		return QuinticEaseOut(val);
	case EaseInOutQuintic:
		return QuinticEaseInOut(val);
	case EaseInSine:
		return SineEaseIn(val);
	case EaseOutSine:
		return SineEaseOut(val);
	case EaseInOutSine:
		return SineEaseInOut(val);
	case EaseInCircular:
		return CircularEaseIn(val);
	case EaseOutCircular:
		return CircularEaseOut(val);
	case EaseInOutCircular:
		return CircularEaseInOut(val);
	case EaseInExponential:
		return ExponentialEaseIn(val);
	case EaseOutExponential:
		return ExponentialEaseOut(val);
	case EaseInOutExponential:
		return ExponentialEaseInOut(val);
	case EaseInElastic:
		return ElasticEaseIn(val);
	case EaseOutElastic:
		return ElasticEaseOut(val);
	case EaseInOutElastic:
		return ElasticEaseInOut(val);
	case EaseInBack:
		return BackEaseIn(val);
	case EaseOutBack:
		return BackEaseOut(val);
	case EaseInOutBack:
		return BackEaseInOut(val);
	case EaseInBounce:
		return BounceEaseIn(val);
	case EaseOutBounce:
		return BounceEaseOut(val);
	case EaseInOutBounce:
		return BounceEaseInOut(val);
	default:
		return val;
	}
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
		progress = applyEase(progress, ease->type);

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