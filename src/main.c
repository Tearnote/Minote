#include "main.h"

#include <stdlib.h>

#include "util.h"
#include "log.h"
#include "window.h"

Log* applog = null;

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

	windowInit(applog);
}

static void cleanup(void)
{
	windowCleanup();
	if (applog) {
		logDestroy(applog);
		applog = null;
	}
	logCleanup();
}

int main(int argc, char* argv[argc + 1])
{
	atexit(cleanup);
	init();

	Window* window = windowCreate(APP_NAME u8" " APP_VERSION,
			(Size2i){1280, 720}, false);

	while (windowIsOpen(window))
		windowPoll();

	windowDestroy(window);
	window = null;

	return EXIT_SUCCESS;
}
