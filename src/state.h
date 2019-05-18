#ifndef STATE_H
#define STATE_H

#include <stdbool.h>

#include "thread.h"

typedef struct {
	bool running;
} state;

extern state* game;
extern mutex stateMutex;

void initState(void);
void cleanupState(void);
#define isRunning() syncBoolRead(&game->running, &stateMutex)
#define setRunning(x) syncBoolWrite(&game->running, (x), &stateMutex)

#endif