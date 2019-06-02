#ifndef STATE_H
#define STATE_H

#include <stdbool.h>

#include "thread.h"
#include "gameplay.h"

typedef struct {
	bool running;
	gameState* game;
} appState;

extern appState* app;
extern mutex runningMutex;
extern mutex gameMutex;

void initState(void);
void cleanupState(void);
#define isRunning() syncBoolRead(&app->running, &runningMutex)
#define setRunning(x) syncBoolWrite(&app->running, (x), &runningMutex)

#endif