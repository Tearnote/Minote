// Minote - effects.h
// A fifo for communicating gfx activations from logic to render thread

#ifndef EFFECTS_H
#define EFFECTS_H

#include "fifo.h"
#include "thread.h"

enum effectType {
	EffectNone,
	EffectLockFlash,
	EffectSize
};

struct effect {
	enum effectType type;
	void *data;
};

extern fifo *effects;
extern mutex effectsMutex;

void initEffects(void);
void cleanupEffects(void);

#define enqueueEffect(e) \
        syncFifoEnqueue(effects, (e), &effectsMutex)
#define dequeueEffect() \
        syncFifoDequeue(effects, &effectsMutex)

#endif //EFFECTS_H
