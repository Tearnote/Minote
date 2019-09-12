// Minote - thread.c

#include "thread.h"

#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#include <pthread.h>
#include <errno.h>

#include "log.h"

void spawnThread(pthread_t *id, void *(*func)(void *),
                 void *arg, const char *name)
{
	if (pthread_create(id, NULL, func, arg) != 0) {
		logCrit("Could not spawn thread %U: %s", name, strerror(errno));
		exit(1);
	}
}

void awaitThread(thread id)
{
	pthread_join(id, NULL);
}

void lockMutex(mutex *lock)
{
	if (!lock)
		return;
	pthread_mutex_lock(lock);
}

void unlockMutex(mutex *lock)
{
	if (!lock)
		return;
	pthread_mutex_unlock(lock);
}

bool syncBoolRead(bool *var, mutex *lock)
{
	lockMutex(lock);
	bool result = *var;
	unlockMutex(lock);
	return result;
}

void syncBoolWrite(bool *var, bool val, mutex *lock)
{
	lockMutex(lock);
	*var = val;
	unlockMutex(lock);
}

void syncFifoEnqueue(fifo *f, void *data, mutex *lock)
{
	lockMutex(lock);
	enqueueFifo(f, data);
	unlockMutex(lock);
}

void *syncFifoDequeue(fifo *f, mutex *lock)
{
	lockMutex(lock);
	void *data = dequeueFifo(f);
	unlockMutex(lock);
	return data;
}
