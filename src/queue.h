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
typedef struct queue queue;

/**
 * Create a new ::queue instance.
 * @param elementSize Size of each queue element in bytes
 * @param maxElements The highest number of elements the queue will be able to
 * store
 * @return A newly created ::queue. Needs to be destroyed with queueDestroy()
 */
queue* queueCreate(size_t elementSize, size_t maxElements);

/**
 * Destroy a ::queue instance. The destroyed object cannot be used anymore and
 * the pointer becomes invalid.
 * @param q The ::queue object
 */
void queueDestroy(queue* q);

/**
 * Add an element to the back of a ::queue. If there is no space, nothing
 * happens.
 * @param q The ::queue object
 * @param element Element data that will be copied into the ::queue
 * @return true if successful, false if out of space
 */
bool queueEnqueue(queue* q, void* element);

/**
 * Remove and return an element from the front of a ::queue. If the queue is
 * empty, nothing happens.
 * @param q The ::queue object
 * @param[out] element Address to rewrite with the removed element
 * @return true if successful, false if queue is empty
 */
bool queueDequeue(queue* q, void* element);

/**
 * Return the element from the front of a ::queue without removing it. If the
 * queue is empty, nothing happens.
 * @param q The ::queue object
 * @param[out] element Address to rewrite with the peeked element
 * @return true if successful, false if queue is empty
 */
bool queuePeek(queue* q, void* element);

/**
 * Check whether a ::queue is empty.
 * @param q The ::queue object
 * @return true if empty, false if not empty
 */
bool queueIsEmpty(queue* q);

/**
 * Check whether a ::queue is full.
 * @param q The ::queue object
 * @return true if full, false if not full
 */
bool queueIsFull(queue* q);

/**
 * Clear a ::queue, setting the number of elements to zero.
 * @param q The ::queue object
 */
void queueClear(queue* q);

#endif //MINOTE_QUEUE_H
