/**
 * Game state - play
 * @file
 */

#ifndef MINOTE_PLAY_H
#define MINOTE_PLAY_H

/**
 * Initialize the play state.
 */
void playInit(void);

/**
 * Clean up the play state. Play functions cannot be used until playInit() is
 * called again.
 */
void playCleanup(void);

/**
 * Advance the play state.
 */
void playUpdate(void);

/**
 * Draw the play state to the screen.
 */
void playDraw(void);

#endif //MINOTE_PLAY_H
