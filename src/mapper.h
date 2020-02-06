/**
 * Converter of raw inputs into player inputs
 * @file
 */

#ifndef MINOTE_MAPPER_H
#define MINOTE_MAPPER_H

#include <stdbool.h>
#include "time.h"

/// Types of player inputs accepted by the game
typedef enum InputType {
	InputNone, ///< zero value
	InputLeft, InputRight, InputUp, InputDown,
	InputButton1, InputButton2, InputButton3, InputButton4,
	InputStart, InputQuit,
	InputSize ///< terminator
} InputType;

/// Action taken by an input
typedef enum InputAction {
	ActionNone, ///< zero value
	ActionPressed,
	ActionReleased,
	ActionSize ///< terminator
} InputAction;

/// A logical input converted from raw device inputs via mappings
typedef struct Input {
	InputType type;
	InputAction action;
	nsec timestamp;
} GameInput;

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
 * Remove and return the next ::GameInput from the mapper's queue.
 * @param[out] element Address to rewrite with the removed input
 * @return true if successful, false if no inputs left
 */
bool mapperDequeue(GameInput* input);

/**
 * Return the ::GameInput from the front of the mapper's queue without
 * removing it. If there are no inputs left, nothing happens.
 * @param[out] element Address to rewrite with the peeked input
 * @return true if successful, false if no inputs left
 */
bool mapperPeek(GameInput* input);

#endif //MINOTE_MAPPER_H
