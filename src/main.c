/**
 * Entry point of the game
 * @file
 */

#include "main.h"

#include <stdlib.h>
#include "util.h"
#include "thread.h"
#include "log.h"
#include "window.h"
#include "game.h"
#include "system.h"
#include "time.h"

/// Frequency of input polling, in Hz
#define InputFrequency 240

/// Main log file of the application
static Log* applog = null;

/**
 * Initialize all game systems. This should be relatively fast and not load
 * too many resources from disk.
 */
static void init(void)
{
	logInit();
	applog = logCreate();
	logEnableConsole(applog);
#ifdef NDEBUG
	const char* logfile = u8"minote.log";
#else //NDEBUG
	const char* logfile = u8"minote-debug.log";
	logSetLevel(applog, LogTrace);
#endif //NDEBUG
	logEnableFile(applog, logfile);
	logInfo(applog, u8"Starting up %s %s", APP_NAME, APP_VERSION);

	systemInit(applog);
}

/**
 * Cleanup all game systems, in reverse initialization order. Do not assume
 * init() was called earlier, or that it was either not called or fully
 * completed. This function must not fail.
 */
static void cleanup(void)
{
	systemCleanup();
	if (applog) {
		logDestroy(applog);
		applog = null;
	}
	logCleanup();
}

/**
 * Entry point function. Initializes systems and spawns other threads. Itself
 * becomes the input handling thread.
 * @return EXIT_SUCCESS on successful execution, EXIT_FAILURE on a handled
 * critical error, other values on unhandled error
 */
int main(int argc, char* argv[argc + 1])
{
	atexit(cleanup);
	init();

	Window* window = windowCreate(APP_NAME u8" " APP_VERSION,
			(Size2i){1280, 720}, false);
	thread* gameThread = threadCreate(game, &(GameArgs){
			.log = applog,
			.window = window
	});

	nsec nextPoll = getTime();
	while (windowIsOpen(window)) {
		nextPoll += secToNsec(1) / InputFrequency;
		windowPoll();
		sleepUntil(nextPoll);
	}

	threadDestroy(gameThread);
	gameThread = null;
	windowDestroy(window);
	window = null;
	return EXIT_SUCCESS;
}
