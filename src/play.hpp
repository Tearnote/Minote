/**
 * Layer: play
 * @file
 * Wrapper for gamemode sublayers. Simulates their logic frames at a correct
 * framerate.
 */

#ifndef MINOTE_PLAY_H
#define MINOTE_PLAY_H

#include "sys/window.hpp"
#include "engine/engine.hpp"
#include "engine/mapper.hpp"

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
void playUpdate(minote::Window& window, minote::Mapper& mapper);

/**
 * Draw the play layer to the screen.
 */
void playDraw(minote::Engine& engine);

#endif //MINOTE_PLAY_H
