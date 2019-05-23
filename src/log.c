#include "log.h"

#include "glad/glad.h"
#include <GLFW/glfw3.h>

#include <stdbool.h>
#include <stdarg.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <time.h>

#include "thread.h"

#ifdef _DEBUG
#define LOG_FILENAME "minote-debug.log"
#else
#define LOG_FILENAME "minote.log"
#endif

static mutex logMutex = newMutex;

static FILE* logFile = NULL;
static const char* prioStrings[] = {"NONE", "DEBUG", "INFO", "WARN", "ERROR", "CRIT"};
static bool printToLogFile = true;
#ifdef _DEBUG
static bool printToStderr = true;
static int logLevel = 1;
#else
static bool printToStderr = false;
static int logLevel = 2;
#endif

static const char* GLFWerror = NULL;

void initLogging() {
	if(printToLogFile) {
		logFile = fopen(LOG_FILENAME, "w");
		if(logFile == NULL) { // On error force stderr logging despite _DEBUG
			printToLogFile = false;
			printToStderr = true;
			logError("Failed to open " LOG_FILENAME " for writing: %s", strerror(errno));
		}
	}
}

void cleanupLogging() {
	if(logFile != NULL) {
		fclose(logFile);
		logFile = NULL;
	}
}

static void logTo(FILE* file, int prio, const char* fmt, va_list ap) {
	lockMutex(&logMutex);
	time_t epochtime;
	struct tm* timeinfo;
	time(&epochtime);
	timeinfo = localtime(&epochtime);
	fprintf(file, "%02d:%02d:%02d [%s] ", timeinfo->tm_hour,
	                                      timeinfo->tm_min,
	                                      timeinfo->tm_sec,
	                                      prioStrings[prio]);
	vfprintf(file, fmt, ap);
	putc('\n', file);
	unlockMutex(&logMutex);
}

void logPrio(int prio, const char* fmt, ...) {
	if(prio < logLevel) return;
	va_list ap;
	va_start(ap, fmt);
	if(printToLogFile)
		logTo(logFile, prio, fmt, ap);
	if(printToStderr)
		logTo(stderr, prio, fmt, ap);
	va_end(ap);
}

void logPrioGLFW(int prio, const char* msg) {
	glfwGetError(&GLFWerror);
	logPrio(prio, "%s: %s", msg, GLFWerror != NULL ? GLFWerror : "Unknown error");
}