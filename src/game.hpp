/**
 * Simulation and rendering thread (game thread)
 * @file
 * Most of the work is done here. Simulation is advanced using collectedInputs gathered
 * by the input thread, and rendering blocks on vsync to provide rate control.
 */

#ifndef MINOTE_GAME_H
#define MINOTE_GAME_H

#include "sys/window.hpp"

/**
 * Entry point of the game thread. The thread will exit after window.isClosing()
 * returns true.
 */
void game(minote::Window& window);

#endif //MINOTE_GAME_H
