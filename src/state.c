// Minote - state.c

#include "state.h"

#include <stdbool.h>
#include <stdlib.h>

#include "thread.h"
#include "util.h"
#include "gameplay.h"

appState* app;
mutex gameMutex = newMutex;
mutex runningMutex = newMutex;

void initState(void) {
	app = allocate(sizeof(app));
	app->running = true;
	app->game = NULL;
}

void cleanupState(void) {
	free(app);
}