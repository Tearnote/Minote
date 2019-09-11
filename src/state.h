// Minote - state.h
// Manages the logical state of the entire app
// Current states are shutdown and gameplay, so for now it's a bool

#ifndef STATE_H
#define STATE_H

#include <stdbool.h>

#include "thread.h"
#include "gameplay.h"

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