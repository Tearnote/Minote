/**
 * Implementation of log.h
 * @file
 */

#include "log.h"

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
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

/// String representation of each #LogLevel
static const char* levelStrings[LogSize] = {
		[LogTrace] = u8"TRACE",
		[LogDebug] = u8"DEBUG",
		[LogInfo]  = u8"INFO",
		[LogWarn]  = u8"WARN",
		[LogError] = u8"ERROR",
		[LogCrit]  = u8"CRIT",
};

struct Log {
	/// Messages with lower level than this will be ignored
	LogLevel level;

	/// If true, messages are printed to stdout/stderr
	bool consoleEnabled;

	/// If true, messages are printed to #file
	bool fileEnabled;

	/// File handle to write messages into. Is null if #fileEnabled is false
	FILE* file;

	/// The string used to open #file. Is null if #fileEnabled is false
	const char* filepath;
};

/// State of log system initialization
static bool initialized = false;

/**
 * Write a log message to a specified output. Attaches a timestamp and formats
 * the log level.
 * @param file The output file
 * @param level Message level
 * @param fmt Format string in printf syntax
 * @param ap List of arguments to print
 */
static void logTo(FILE* file, LogLevel level, const char* fmt, va_list* ap)
{
	time_t epochtime = time(null);
	struct tm* timeinfo = localtime(&epochtime);
	//TODO turn into a mutex instead
	//flockfile(file);
	int result = fprintf(file, u8"%02d:%02d:%02d [%s] ",
			timeinfo->tm_hour, timeinfo->tm_min, timeinfo->tm_sec,
			levelStrings[level]);
	if (result < 0) {
		perror(u8"Failed to write into log file");
		errno = 0;
	}
	// A copy of va_list is needed in case this is executed more than once per
	// log function call
	va_list apCopy;
	va_copy(apCopy, *ap);
	result = vfprintf(file, fmt, apCopy);
	if (result < 0) {
		perror(u8"Failed to write into log file");
		errno = 0;
	}
	result = fputc('\n', file);
	//TODO turn into a mutex instead
	//funlockfile(file);
	if (result == EOF) {
		perror(u8"Failed to write into log file");
		errno = 0;
	}
}

/**
 * The actual log message handling function. Performs level filtering and
 * output selection.
 * @param l The ::Log object
 * @param level Message level
 * @param fmt Format string in printf syntax
 * @param ap List of arguments to print
 */
static void logPrio(Log* l, LogLevel level, const char* fmt, va_list* ap)
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
	SetConsoleOutputCP(65001); // Set Windows cmd output encoding to UTF-8
#else //_WIN32
	setlocale(LC_ALL, ""); // Switch from C locale to system locale
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
	Log* l = alloc(sizeof(Log));
	l->level = LogInfo;
	return l;
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
		logError(l, u8"Failed to open %s for writing: %s",
				filepath, strerror(errno));
		errno = 0;
		if (!consoleEnabled)
			logDisableConsole(l);
	} else {
		l->fileEnabled = true;
		l->filepath = filepath;
	}
	assert(l->fileEnabled == (l->file != null));
	assert(l->fileEnabled == (l->filepath != null));
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
		logError(l, u8"Failed to flush %s: %s",
				l->filepath, strerror(errno));
		errno = 0;
		if (!consoleEnabled)
			logDisableConsole(l);
	}
	l->filepath = null;
	l->fileEnabled = false;
	assert(l->fileEnabled == (l->file != null));
	assert(l->fileEnabled == (l->filepath != null));
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
	logPrio(l, LogTrace, fmt, &ap);
	va_end(ap);
}

void logDebug(Log* l, const char* fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);
	logPrio(l, LogDebug, fmt, &ap);
	va_end(ap);
}

void logInfo(Log* l, const char* fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);
	logPrio(l, LogInfo, fmt, &ap);
	va_end(ap);
}

void logWarn(Log* l, const char* fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);
	logPrio(l, LogWarn, fmt, &ap);
	va_end(ap);
}

void logError(Log* l, const char* fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);
	logPrio(l, LogError, fmt, &ap);
	va_end(ap);
}

void logCrit(Log* l, const char* fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);
	logPrio(l, LogCrit, fmt, &ap);
	va_end(ap);
}
