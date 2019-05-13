#ifndef THREADS_H
#define THREADS_H

#include <stdbool.h>

#include <pthread.h>

typedef pthread_t thread;
typedef pthread_mutex_t mutex;
#define newMutex PTHREAD_MUTEX_INITIALIZER

void spawnThread(thread* id, void *(* func)(void *), void* arg, const char* name);
void awaitThread(thread id);

void lockMutex(mutex* lock);
void unlockMutex(mutex* lock);

// Convenience methods for synchronized getters/setters
bool syncBoolRead(bool* var, mutex* lock);
void syncBoolWrite(bool* var, bool val, mutex* lock);

#endif