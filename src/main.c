#include "main.h"

#include "log.h"
#include "window.h"
#include "input.h"
#include "state.h"
#include "render.h"

void cleanup() {
	cleanupInput();
	cleanupWindow();
	cleanupLogging();
}

int main() {
	initLogging();
	logInfo("Starting up " APP_NAME " " APP_VERSION);
	atexit(cleanup);
	initWindow();
	initInput();
	
	spawnRenderer();
	//spawnEngine();
	
	// Main thread's loop. Handle input updates
	while(isRunning()) {
		updateInput();
		sleepInput(); // Sleep the thread between updates to avoid wasting CPU
	}
	
	// All other threads loop on isRunning(), so it's safe to wait on them here
	//awaitEngine();
	awaitRenderer();
	
	return 0;
}