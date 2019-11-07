// Minote - global/effects.h
// A fifo for communicating gfx activations from logic to render thread
//TODO Turn into generic observer pattern
//TODO https://gameprogrammingpatterns.com/observer.html

#ifndef GLOBAL_EFFECTS_H
#define GLOBAL_EFFECTS_H

#include <stdbool.h>

#include "types/fifo.h"
#include "types/mino.h"
#include "util/thread.h"

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
	void *data; // One of the below structs or NULL
};

struct lineClearEffectData {
	int lines;
	int combo;
	enum mino playfield[PLAYFIELD_H][PLAYFIELD_W];
	bool clearedLines[PLAYFIELD_H];
};

struct thumpEffectData {
	int x, y;
};

struct slideEffectData {
	int x, y;
	int direction; // -1 for left, 1 for right
	bool strong; // true for a DAS shift
};

void initEffects(void);
void cleanupEffects(void);

void enqueueEffect(struct effect *e);
struct effect *dequeueEffect(void);

#endif //GLOBAL_EFFECTS_H