/**
 * Simulation and rendering thread (game thread)
 * @file
 * Most of the work is done here. Game logic is advanced using inputs gathered
 * by the main thread, and rendering blocks on vsync to provide rate control.
 */

#pragma once

#include "sys/window.hpp"

namespace minote {

/**
 * Entry point of the game thread. The thread will exit after window.isClosing()
 * returns true.
 * @param window Window to run the game on, with disabled context
 */
void game(Window& window);

}
