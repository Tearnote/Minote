#include "main.h"

#include "log.h"
#include "window.h"
#include "input.h"
#include "state.h"
#include "render.h"
#include "logic.h"
#include "timer.h"

void cleanup() {
	cleanupInput();
	cleanupWindow();
	cleanupTimer();
	cleanupLogging();
}

int main() {
	initLogging();
	logInfo("Starting up " APP_NAME " " APP_VERSION);
	atexit(cleanup);
	initTimer();
	initWindow();
	initInput();
	
	spawnRenderer();
	spawnLogic();
	
	// Main thread's loop. Handle input updates
	while(isRunning()) {
		updateInput();
		sleepInput(); // Sleep the thread between updates to avoid wasting CPU
	}
	
	// All other threads loop on isRunning(), so it's safe to wait on them here
	awaitLogic();
	awaitRenderer();
	
	return 0;
}