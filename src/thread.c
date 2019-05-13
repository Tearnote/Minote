#include "thread.h"

#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#include <pthread.h>
#include <errno.h>

#include "log.h"

void spawnThread(pthread_t* id, void *(* func)(void *), void* arg, const char* name) {
	if(pthread_create(id, NULL, func, arg) != 0) {
		logCrit("Could not spawn thread %s: %s", name, strerror(errno));
		exit(1);
	}
}

void awaitThread(thread id) {
	pthread_join(id, NULL);
}

void lockMutex(mutex* lock) {
	pthread_mutex_lock(lock);
}

void unlockMutex(mutex* lock) {
	pthread_mutex_unlock(lock);
}

bool syncBoolRead(bool* var, mutex* lock) {
	lockMutex(lock);
	bool result = *var; // Cannot be static to keep the function reentrant
	unlockMutex(lock);
	return result;
}

void syncBoolWrite(bool* var, bool val, mutex* lock) {
	lockMutex(lock);
	*var = val;
	unlockMutex(lock);
}