// Minote - main.c
// Entry point
// Spawns threads, itself is the input handling thread

#include "main.h"

#include "util/log.h"
#include "main/window.h"
#include "main/poll.h"
#include "global/input.h"
#include "global/state.h"
#include "render/render.h"
#include "logic/logic.h"
#include "util/timer.h"
#include "global/settings.h"
#include "global/effects.h"

static void cleanup(void)
{
	cleanupEffects();
	cleanupInput();
	cleanupWindow();
	cleanupState();
	cleanupLogging();
	cleanupSettings();
}

int main(int argc, char *argv[])
{
	atexit(cleanup);
	initLogging();
	logInfo("Starting up %U %U", APP_NAME, APP_VERSION);
	initSettings();
	loadSwitchSettings(argc, argv);
	initState();
	initWindow();
	initInput();
	initEffects();

	spawnRenderer();
	spawnLogic();

	// Main thread's loop, handles input updates and window events
	initPoll();
	while (isRunning()) {
		updatePoll();
		sleepPoll();
	}
	cleanupPoll();

	// Other threads loop on isRunning(), so it's safe to wait on them here
	awaitLogic();
	awaitRenderer();

	return 0;
}