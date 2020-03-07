/**
 * Layer: play
 * @file
 * Wrapper for gamemode sublayers. Simulates their logic frames at a correct
 * framerate.
 */

#ifndef MINOTE_PLAY_H
#define MINOTE_PLAY_H

/**
 * Initialize the play layer. Needs to be called before the layer can be used.
 */
void playInit(void);

/**
 * Clean up the play layer. Play functions cannot be used until playInit() is
 * called again.
 */
void playCleanup(void);

/**
 * Advance the play layer.
 */
void playUpdate(void);

/**
 * Draw the play layer's geometry - scene, blocks and other bits.
 */
void playDrawGeometry(void);

/**
 * Draw the play layer's text - mostly the HUD.
 */
void playDrawText(void);

#endif //MINOTE_PLAY_H
