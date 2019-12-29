/**
 * Simulation and rendering thread (game thread)
 * @file
 * Most of the work is done here. Simulation is advanced using inputs gathered
 * by the input thread, and rendering blocks on vsync to provide rate control.
 */

#ifndef MINOTE_GAME_H
#define MINOTE_GAME_H

#include "window.h"

/// Struct holding arguments to the game() thread
typedef struct GameArgs {
	Log* log; ///< ::Log to use by the thread. Must not be in use anywhere else
	Window* window; ///< The ::Window to draw the game into
} GameArgs;

/**
 * Entry point of the game thread. Spawn this only after all systems are
 * initialized. The thread will exit when #windowIsOpen(@a arg) returns false.
 * @param args Pointer to a ::GameArgs struct
 * @return Unused, always null
 */
void* game(void* args);

#endif //MINOTE_GAME_H
