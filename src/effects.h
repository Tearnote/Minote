// Minote - effects.h
// A fifo for communicating gfx activations from logic to render thread

#ifndef EFFECTS_H
#define EFFECTS_H

#include <stdbool.h>

#include "fifo.h"
#include "thread.h"
#include "gameplay.h"

enum effectType {
	EffectNone,
	EffectLockFlash,
	EffectLineClear,
	EffectSize
};

struct effect {
	enum effectType type;
	void *data;
};

struct lineClearData {
	int lines;
	float speed;
	enum mino playfield[PLAYFIELD_H][PLAYFIELD_W];
	bool clearedLines[PLAYFIELD_H];
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
