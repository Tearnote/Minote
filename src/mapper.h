/**
 * Converter of raw collectedInputs into player collectedInputs
 * @file
 */

#ifndef MINOTE_MAPPER_H
#define MINOTE_MAPPER_H

#include <stdbool.h>
#include "time.h"

typedef enum InputType {
	InputNone, ///< zero value
	InputLeft, InputRight, InputUp, InputDown,
	InputButton1, InputButton2, InputButton3, InputButton4,
	InputStart, InputQuit,
	InputSize ///< terminator
} InputType;

/// A logical input converted from raw device collectedInputs via mappings
typedef struct Input {
	InputType type;
	bool state; ///< true if press, false if release
	nsec timestamp;
} Input;

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
 * Process all pending collectedInputs from the window and insert them into the player
 * event queue.
 */
void mapperUpdate(void);

/**
 * Remove and return the next ::Input from the mapper's queue.
 * @param[out] element Address to rewrite with the removed input
 * @return true if successful, false if no collectedInputs left
 */
bool mapperDequeue(Input* input);

/**
 * Return the ::Input from the front of the mapper's queue without
 * removing it. If there are no collectedInputs left, nothing happens.
 * @param[out] element Address to rewrite with the peeked input
 * @return true if successful, false if no collectedInputs left
 */
bool mapperPeek(Input* input);

#endif //MINOTE_MAPPER_H