/**
 * A constant-size FIFO queue container
 * @file
 * Based on a circular buffer, tightly packed. Data is copied on insertion and
 * retrieval, so it is best used for passing small messages. All functions in
 * this interface are O(1).
 */

#ifndef MINOTE_QUEUE_H
#define MINOTE_QUEUE_H

#include <stdbool.h>
#include <stddef.h>

/// Opaque queue container. You can obtain an instance with queueCreate().
typedef struct Queue Queue;

/**
 * Create a new ::Queue instance.
 * @param elementSize The size of each queue element in bytes
 * @param maxElements The highest number of elements the queue will be able to
 * store
 * @return A newly created ::Queue. Needs to be destroyed with queueDestroy()
 */
Queue* queueCreate(size_t elementSize, size_t maxElements);

/**
 * Destroy a ::Queue instance. The destroyed object cannot be used anymore and
 * the pointer becomes invalid.
 * @param q The ::Queue object
 */
void queueDestroy(Queue* q);

/**
 * Add an element to the back of a ::Queue. If there is no space, nothing
 * happens.
 * @param q The ::Queue object
 * @param element Element data that will be copied into the ::Queue
 * @return true if successful, false if out of space
 */
bool queueEnqueue(Queue* q, void* element);

/**
 * Remove and return an element from the front of a ::Queue. If the queue is
 * empty, nothing happens.
 * @param q The ::Queue object
 * @param[out] element Address to rewrite with the removed element
 * @return true if successful, false if queue is empty
 */
bool queueDequeue(Queue* q, void* element);

/**
 * Return the element from the front of a ::Queue without removing it. If the
 * queue is empty, nothing happens.
 * @param q The ::Queue object
 * @param[out] element Address to rewrite with the peeked element
 * @return true if successful, false if queue is empty
 */
bool queuePeek(Queue* q, void* element);

/**
 * Check whether a ::Queue is empty.
 * @param q The ::Queue object
 * @return true if empty, false if not empty
 */
bool queueIsEmpty(Queue* q);

/**
 * Check whether a ::Queue is full.
 * @param q The ::Queue object
 * @return true if full, false if not full
 */
bool queueIsFull(Queue* q);

/**
 * Clear a ::Queue, setting the number of elements to zero.
 * @param q The ::Queue object
 */
void queueClear(Queue* q);

#endif //MINOTE_QUEUE_H
