/**
 * Entry point of the game
 * @file
 */

#include "main.h"

#include <stdlib.h>
#include "thread.h"
#include "window.h"
#include "system.h"
#include "util.h"
#include "game.h"
#include "time.h"
#include "log.h"

/**
 * Initialize all game systems. This should be relatively fast and not load
 * too many resources from disk.
 */
static void init(void)
{
	logInit();
	logEnableConsole(applog);
#ifdef NDEBUG
	const char* logfile = u8"minote.log";
#else //NDEBUG
	const char* logfile = u8"minote-debug.log";
	logSetLevel(applog, LogTrace);
#endif //NDEBUG
	logEnableFile(applog, logfile);
	logInfo(applog, u8"Starting up %s %s", AppName, AppVersion);

	systemInit();
	windowInit(AppName u8" " AppVersion, (size2i){1280, 720}, false);
}

/**
 * Cleanup all game systems, in reverse initialization order. Do not assume
 * init() was called earlier, or that it was either not called or fully
 * completed. This function must not fail.
 */
static void cleanup(void)
{
	windowCleanup();
	systemCleanup();
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

	thread* gameThread = threadCreate(game, null);

	int measuredSecond = getTime() / secToNsec(1);
	int frameCount = 0;
	while (windowIsOpen()) {
		int currentSecond = getTime() / secToNsec(1);
		if (currentSecond == measuredSecond) {
			frameCount += 1;
		} else {
			frameCount = 0;
			measuredSecond = currentSecond;
		}
		windowPoll();
		sleepFor(secToNsec(1) / 1000); // 1ms minimum
	}

	threadDestroy(gameThread);
	gameThread = null;
	return EXIT_SUCCESS;
}
