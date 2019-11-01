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
	EffectThump,
	EffectSlide,
	EffectBravo,
	EffectSize
};

struct effect {
	enum effectType type;
	void *data;
};

struct lineClearEffectData {
	int lines;
	int combo;
	enum mino playfield[PLAYFIELD_H][PLAYFIELD_W];
	bool clearedLines[PLAYFIELD_H];
};

struct thumpEffectData {
	int x;
	int y;
};

struct slideEffectData {
	int x;
	int y;
	int direction; // -1 for left, 1 for right
	bool strong;
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
