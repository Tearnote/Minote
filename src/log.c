#include "log.h"

#include <stdbool.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <assert.h>
#include <time.h>
#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#else //_WIN32
#include <locale.h>
#endif //_WIN32
#include "util.h"

static const char* levelStrings[LogSize] = {
		[LogTrace] = "TRACE",
		[LogDebug] = "DEBUG",
		[LogInfo]  = "INFO",
		[LogWarn]  = "WARN",
		[LogError] = "ERROR",
		[LogCrit]  = "CRIT",
};

struct Log {
	LogLevel level;
	bool consoleEnabled;
	bool fileEnabled;
	FILE* file;
	const char* filepath;
};

static bool initialized = false;

static void logTo(FILE* file, LogLevel level, const char* fmt, va_list ap)
{
	time_t epochtime = time(null);
	struct tm* timeinfo = localtime(&epochtime);
	int result = fprintf(file, "%02d:%02d:%02d [%s] ",
			timeinfo->tm_hour, timeinfo->tm_min, timeinfo->tm_sec,
			levelStrings[level]);
	if (result < 0) {
		perror("Failed to write into log file");
		errno = 0;
	}
	result = vfprintf(file, fmt, ap);
	if (result < 0) {
		perror("Failed to write into log file");
		errno = 0;
	}
	result = fputc('\n', file);
	if (result == EOF) {
		perror("Failed to write into log file");
		errno = 0;
	}
}

static void logPrio(Log* l, LogLevel level, const char* fmt, va_list ap)
{
	assert(initialized);
	assert(l);
	assert(level > LogNone && level < LogSize);
	assert(fmt);
	if (level < l->level) return;
	if (l->consoleEnabled) {
		FILE* output = (level >= LogWarn ? stderr : stdout);
		logTo(output, level, fmt, ap);
	}
	if (l->fileEnabled)
		logTo(l->file, level, fmt, ap);
}

void logInit(void)
{
	if (initialized) return;
#ifdef _WIN32
	SetConsoleOutputCP(65001);
#else //_WIN32
	setlocale(LC_ALL, "");
#endif //_WIN32
	initialized = true;
}

void logCleanup(void)
{
	if (!initialized) return;
	// Do nothing... for now.
	initialized = false;
}

Log* logCreate(void)
{
	assert(initialized);
	Log* result = alloc(sizeof(Log));
	result->level = LogInfo;
	return result;
}

void logDestroy(Log* l)
{
	assert(initialized);
	assert(l);
	if (l->fileEnabled)
		logDisableFile(l);
	free(l);
}

void logEnableConsole(Log* l)
{
	assert(initialized);
	assert(l);
	l->consoleEnabled = true;
}

void logEnableFile(Log* l, const char* filepath)
{
	assert(initialized);
	assert(l);
	assert(filepath);
	if (l->fileEnabled) return;
	l->file = fopen(filepath, "w");
	if (!l->file) {
		bool consoleEnabled = l->consoleEnabled;
		logEnableConsole(l);
		logError(l, "Failed to open %s for writing: %s",
				filepath, strerror(errno));
		errno = 0;
		if (!consoleEnabled)
			logDisableConsole(l);
	} else {
		l->fileEnabled = true;
		l->filepath = filepath;
	}
	assert(l->fileEnabled == !!l->file && l->fileEnabled == !!l->filepath);
}

void logDisableConsole(Log* l)
{
	assert(initialized);
	assert(l);
	l->consoleEnabled = false;
}

void logDisableFile(Log* l)
{
	assert(initialized);
	assert(l);
	if (!l->fileEnabled) return;
	int result = 0;
	result = fclose(l->file);
	l->file = null;
	if (result) {
		bool consoleEnabled = l->consoleEnabled;
		logEnableConsole(l);
		logError(l, "Failed to flush %s: %s",
				l->filepath, strerror(errno));
		errno = 0;
		if (!consoleEnabled)
			logDisableConsole(l);
	}
	l->filepath = null;
	l->fileEnabled = false;
	assert(l->fileEnabled == !!l->file && l->fileEnabled == !!l->filepath);
}

void logSetLevel(Log* l, LogLevel level)
{
	assert(initialized);
	assert(l);
	assert(level > LogNone && level < LogSize);
	l->level = level;
}

void logTrace(Log* l, const char* fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);
	logPrio(l, LogTrace, fmt, ap);
	va_end(ap);
}

void logDebug(Log* l, const char* fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);
	logPrio(l, LogDebug, fmt, ap);
	va_end(ap);
}

void logInfo(Log* l, const char* fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);
	logPrio(l, LogInfo, fmt, ap);
	va_end(ap);
}

void logWarn(Log* l, const char* fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);
	logPrio(l, LogWarn, fmt, ap);
	va_end(ap);
}

void logError(Log* l, const char* fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);
	logPrio(l, LogError, fmt, ap);
	va_end(ap);
}

void logCrit(Log* l, const char* fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);
	logPrio(l, LogCrit, fmt, ap);
	va_end(ap);
}
