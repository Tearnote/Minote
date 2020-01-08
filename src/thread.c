/**
 * Implementation of thread.h
 * @file
 */

#include "thread.h"

#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <stdio.h>
#include "util.h"

thread* threadCreate(void* func(void*), void* arg)
{
	assert(func);
	thread* t = alloc(sizeof(*t));
	int error = pthread_create(t, null, func, arg);
	if (error) {
		fprintf(stderr, u8"Could not create thread: %s", strerror(error));
		exit(EXIT_FAILURE);
	}
	return t;
}

void threadDestroy(thread* t)
{
	assert(t);
	pthread_join(*t, null);
	free(t);
	t = null;
}

mutex* mutexCreate(void)
{
	mutex* m = alloc(sizeof(*m));
	int error = pthread_mutex_init(m, null);
	if (error) {
		fprintf(stderr, u8"Could not create mutex: %s", strerror(error));
		exit(EXIT_FAILURE);
	}
	return m;
}

void mutexDestroy(mutex* m)
{
	assert(m);
	int error = pthread_mutex_destroy(m);
	if (error) {
		fprintf(stderr, u8"Could not destroy mutex: %s", strerror(error));
		exit(EXIT_FAILURE);
	}
	free(m);
	m = null;
}

void mutexLock(mutex* m)
{
	assert(m);
	pthread_mutex_lock(m);
}

void mutexUnlock(mutex* m)
{
	assert(m);
	pthread_mutex_unlock(m);
}
