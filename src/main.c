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

/// Frequency of input polling, in Hz
#define InputFrequency 480.0
/// Inverse of #InputFrequency, in ::nsec
#define InputTick (secToNsec(1) / InputFrequency)

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

	nsec nextPoll = getTime();
	while (windowIsOpen()) {
		nextPoll += InputTick;
		windowPoll();
		sleepUntil(nextPoll);
	}

	threadDestroy(gameThread);
	gameThread = null;
	return EXIT_SUCCESS;
}
