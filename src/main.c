// Minote - main.c
// Entry point
// Spawns threads, itself is the input handling thread

#include "main.h"

#include <stdlib.h>

#include "log.h"
#include "window.h"
#include "input.h"
#include "state.h"
#include "render.h"
#include "logic.h"
#include "timer.h"

static void cleanup(void)
{
	cleanupInput();
	cleanupWindow();
	cleanupState();
	cleanupTimer();
	cleanupLogging();
}

int main(void)
{
	initLogging();
	logInfo("Starting up %s %s", APP_NAME, APP_VERSION);
	atexit(cleanup);
	initTimer();
	initState();
	initWindow();
	initInput();

	spawnRenderer();
	spawnLogic();

	// Main thread's loop, handles input updates and window events
	while (isRunning()) {
		updateInput();
		sleepInput();
	}

	// Other threads loop on isRunning(), so it's safe to wait on them here
	awaitLogic();
	awaitRenderer();

	return 0;
}