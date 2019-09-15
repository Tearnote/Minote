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

void printUsage(const char *invalid)
{
	if (invalid)
		printf("ERROR: Invalid argument: %s\n\n", invalid);
	puts("Minote [ OPTIONS ]");
	puts("");
	puts("Available options:");
	puts("    --help: Print this message");
	puts("    --replay: Open replay.mre in replay viewer");
	puts("    --fullscreen: Use exclusive fullscreen mode");
}

static void cleanup(void)
{
	cleanupInput();
	cleanupWindow();
	cleanupState();
	cleanupTimer();
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