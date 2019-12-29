/**
 * Simplified threading primitives
 * @file
 * Currently based on pthreads and C11 atomics, but it should be basic enough to
 * wrap any other OS-specific thread subsystem if needed.
 */

#ifndef MINOTE_THREAD_H
#define MINOTE_THREAD_H

#include <pthread.h>
#include <stdatomic.h>

/**
 * Type representing a thread ID. Becomes defined by being passed to
 * threadCreate() and undefined after threadDestroy() returns.
 */
typedef pthread_t thread;

/**
 * Type representing a mutex lock. Must be initialized with mutexCreate().
 */
typedef pthread_mutex_t mutex;

/// A renaming of _Atomic to be more in line with other language keywords
#define atomic _Atomic

/**
 * Create a new ::thread instance, which immediately runs the specified
 * function in a separate thread of execution.
 * @param func Pointer of the function to run in a concurrent thread
 * @param arg Value of the parameter that will be passed to @a func. Can be null
 * @return A newly created ::thread. Needs to be destroyed with threadDestroy()
 */
thread* threadCreate(void* func(void*), void* arg);

/**
 * Destroy a ::thread instance. If the thread is still running, block execution
 * of current thread until the specified thread exits. The destroyed object
 * cannot be used anymore and the pointer becomes invalid.
 * @param id The ::thread object
 */
void threadDestroy(thread* t);

/**
 * Create a new ::mutex instance.
 * @return A newly created ::mutex. Needs to be destroyed with mutexDestroy()
 */
mutex* mutexCreate(void);

/**
 * Destroy a ::mutex instance. The mutex must not be locked.
 * @param m The ::mutex object
 */
void mutexDestroy(mutex* m);

/**
 * Lock a #mutex or block until it can be locked. Use this to ensure exclusive
 * access to data protected by the mutex. Must be accompanied by a mutexUnlock()
 * as early as possible and without fail.
 * @param m The #mutex to lock
 */
void mutexLock(mutex* m);

/**
 * Unlock a previously locked #mutex.
 * @param m The locked #mutex to unlock
 */
void mutexUnlock(mutex* m);

#endif //MINOTE_THREAD_H
