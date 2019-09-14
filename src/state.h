// Minote - state.h
// Manages the logical state of the entire app

#ifndef STATE_H
#define STATE_H

#include <stdbool.h>

#include "thread.h"
#include "gameplay.h"
#include "replay.h"

enum appState {
	AppNone,
	AppGameplay,
	AppReplay,
	AppShutdown,
	AppSize
};

struct app {
	enum appState state; //SYNC stateMutex getState setState
	struct game *game; //SYNC gameMutex
	struct replay *replay; //SYNC gameMutex
};

extern struct app *app;
extern mutex stateMutex;
extern mutex gameMutex;

void initState(enum appState initial);
void cleanupState(void);

#define isRunning() \
	(getState() != AppShutdown)

enum appState getState(void);
void setState(enum appState state);

#endif // STATE_H