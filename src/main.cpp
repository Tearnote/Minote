/**
 * Game entry point
 * @file
 */

#include "main.hpp"

#include <cstdlib>
#include <clocale>
#include <thread>
#include "window.hpp"
#include "system.hpp"
#include "debug.hpp"
#include "game.hpp"
#include "log.hpp"

/**
 * Entry point function. Initializes systems and spawns other threads. Itself
 * becomes the input handling thread.
 * @return EXIT_SUCCESS on successful execution, EXIT_FAILURE on a handled
 * critical error, other values on unhandled error
 */
auto main(int, char*[]) -> int
{
	//region Initialization

	using minote::AppName;
	using minote::AppVersion;

	// Logging
	std::setlocale(LC_ALL, "C"); // Fix cstdio issues with float parsing and others
	logInit();
	logEnableConsole(applog);
#ifdef NDEBUG
	constexpr char logfile[]{"minote.log"};
#else //NDEBUG
	constexpr char logfile[]{"minote-debug.log"};
	logSetLevel(applog, LogTrace);
#endif //NDEBUG
	logEnableFile(applog, logfile);
	logInfo(applog, "Starting up %s %s", AppName, AppVersion);

	// Window creation
	systemInit();
	char windowTitle[64]{""};
	std::snprintf(windowTitle, 64, "%s %s", AppName, AppVersion);
	windowInit(windowTitle, {1280, 720}, false);
#ifdef MINOTE_DEBUG
	debugInputSetup();
#endif //MINOTE_DEBUG

	//endregion Initialization

	std::thread gameThread{game};

	// Input loop
	while (windowIsOpen()) {
		windowPoll();
		sleepFor(secToNsec(1) / 1000); // 1ms minimum
	}

	gameThread.join();

	// Cleanup
	windowCleanup();
	systemCleanup();
	logCleanup();

	return EXIT_SUCCESS;
}
