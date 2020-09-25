/**
 * A sub-system of mrs.h for drawing the mode's state
 * @file
 */

#ifndef MINOTE_MRSDRAW_H
#define MINOTE_MRSDRAW_H

#include <stdbool.h>

/**
 * Initialize the mrs drawing subsystem. Needs to be called before mrsDraw()
 * can be used.
 */
void mrsDrawInit(void);

/**
 * Clean up mrs drawing. mrsDraw() cannot be used until mrsDrawInit() is
 * called again.
 */
void mrsDrawCleanup(void);

/**
 * Draw the mrs sublayer to the screen.
 */
void mrsDraw(void);

/**
 * Reset a newly spawned piece's draw data. Call right after spawning.
 */
void mrsEffectSpawn(void);

/**
 * Flash the player piece - call as soon as it has locked, after stamping.
 */
void mrsEffectLock(void);

/**
 * Create some pretty particle effects on line clear. Call this before the row
 * is actually cleared.
 * @param row Height of the cleared row
 * @param power Number of lines cleared, which affects the strength
 */
void mrsEffectClear(int row, int power);

/**
 * Create a dust cloud effect on blocks that have fallen on top of other blocks.
 * Use after the relevant fieldDropRow() was called.
 * @param row Position of the row that was cleared
 */
void mrsEffectThump(int row);

/**
 * Create a dust effect under the player piece when it lands on the stack.
 * @param direction -1 if player is currently moving left, 1 if right, 0 if neither
 */
void mrsEffectLand(int direction);

/**
 * Create a friction effect under the player piece as it moves sideways.
 * Use after shift().
 * @param direction Most recently performed shift. -1 for left, 1 for right
 * @param fast true if DAS shift, false if manual
 */
void mrsEffectSlide(int direction, bool fast);

#endif //MINOTE_MRSDRAW_H
