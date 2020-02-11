/**
 * Layer: play
 * @file
 * Wrapper for gamemode sublayers. Simulates their logic frames at a correct
 * framerate.
 */

#ifndef MINOTE_PLAY_H
#define MINOTE_PLAY_H

/**
 * Initialize the play state. Needs to be called before the state can be used.
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
