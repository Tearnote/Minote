// Minote - state.h
// Manages the logical state of the entire app
// Current states are shutdown and gameplay, so for now it's a bool

#ifndef STATE_H
#define STATE_H

#include <stdbool.h>

#include "thread.h"
#include "gameplay.h"

struct appState {
	bool running; //SYNC runningMutex isRunning setRunning
	struct game *game; //SYNC gameMutex
};

extern struct appState *app;
extern mutex runningMutex;
extern mutex gameMutex;

void initState(void);
void cleanupState(void);
#define isRunning() \
        syncBoolRead(&app->running, &runningMutex)
#define setRunning(x) \
        syncBoolWrite(&app->running, (x), &runningMutex)

#endif // STATE_H
