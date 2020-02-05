/**
 * Converter of raw inputs into player inputs
 * @file
 */

#ifndef MINOTE_MAPPER_H
#define MINOTE_MAPPER_H

#include <stdbool.h>

/// A logical input converted from raw device inputs via mappings
typedef struct PlayerInput {
	int key;
	int action;
} PlayerInput;

/**
 * Initialize the mapper system. windowInit() must have been called first.
 * This can be called on a different thread than windowInit().
 */
void mapperInit(void);

/**
 * Cleanup the renderer system. No other mapper functions cannot be used
 * until mapperInit() is called again.
 */
void mapperCleanup(void);

/**
 * Process all pending inputs from the window and insert them into the player
 * event queue.
 */
void mapperUpdate(void);

/**
 * Remove and return the next ::PlayerInput from the mapper's queue.
 * @param[out] element Address to rewrite with the removed input
 * @return true if successful, false if no inputs left
 */
bool mapperDequeue(PlayerInput* input);

/**
 * Return the ::PlayerInput from the front of the mapper's queue without
 * removing it. If there are no inputs left, nothing happens.
 * @param[out] element Address to rewrite with the peeked input
 * @return true if successful, false if no inputs left
 */
bool mapperPeek(PlayerInput* input);

#endif //MINOTE_MAPPER_H
