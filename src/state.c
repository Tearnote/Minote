// Minote - state.c

#include "state.h"

#include <stdbool.h>
#include <stdlib.h>

#include "thread.h"
#include "util.h"
#include "gameplay.h"

struct app *app;
mutex stateMutex = newMutex;
mutex gameMutex = newMutex;
mutex replayMutex = newMutex;

void initState(enum appState initial)
{
	app = allocate(sizeof(*app));
	app->state = initial;
	app->game = NULL;
	app->replay = NULL;
}

void cleanupState(void)
{
	if (!app)
		return;
	free(app);
	app = NULL;
}

enum appState getState(void)
{
	enum appState result;
	lockMutex(&stateMutex);
	result = app->state;
	unlockMutex(&stateMutex);
	return result;
}

void setState(enum appState state)
{
	lockMutex(&stateMutex);
	app->state = state;
	unlockMutex(&stateMutex);
}