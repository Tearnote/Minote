#include "main.h"

#include <stdlib.h>

#include "util.h"
#include "log.h"

Log* applog = null;

static void init(void)
{
	logInit();
	applog = logCreate();
	logEnableConsole(applog);
#ifdef NDEBUG
	const char* logfile = "minote.log";
#else //NDEBUG
	const char* logfile = "minote-debug.log";
	logSetLevel(applog, LogTrace);
#endif //NDEBUG
	logEnableFile(applog, logfile);
	logInfo(applog, u8"Starting up %s %s", APP_NAME, APP_VERSION);
}

static void cleanup(void)
{
	logDestroy(applog);
	applog = null;
	logCleanup();
}

int main(int argc, char* argv[argc + 1])
{
	atexit(cleanup);
	init();
	return EXIT_SUCCESS;
}
