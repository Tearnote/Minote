#ifndef STATE_H
#define STATE_H

#include <stdbool.h>

#include "thread.h"
#include "mino.h"

#define PLAYFIELD_W 10
#define PLAYFIELD_H 20

typedef struct {
	bool running;
	mino playfield[PLAYFIELD_H][PLAYFIELD_W];
} state;

extern state* game;
extern mutex stateMutex;

void initState(void);
void cleanupState(void);
#define isRunning() syncBoolRead(&game->running, &stateMutex)
#define setRunning(x) syncBoolWrite(&game->running, (x), &stateMutex)

#endif