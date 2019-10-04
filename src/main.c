// Minote - main.c
// Entry point
// Spawns threads, itself is the input handling thread

#include "main.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "log.h"
#include "window.h"
#include "input.h"
#include "state.h"
#include "render.h"
#include "logic.h"
#include "timer.h"
#include "settings.h"
#include "effects.h"

void printUsage(const char *invalid)
{
	if (invalid)
		printf("ERROR: Invalid argument: %s\n\n", invalid);
	puts("Minote [ OPTIONS ]");
	puts("");
	puts("Available options:");
	puts("  --help - Print usage help");
	puts("  --replay - Open replay.mre in replay viewer");
	puts("  --fullscreen - Use exclusive fullscreen mode");
	puts("  --nosync - Disable hard GPU sync for higher performance at the cost of latency");
}

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
	while (isRunning()) {
		updateInput();
		sleepInput();
	}

	// Other threads loop on isRunning(), so it's safe to wait on them here
	awaitLogic();
	awaitRenderer();

	return 0;
}