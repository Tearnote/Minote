// Minote - game.hpp
// Simulation and rendering thread (game thread)
// Most of the work is done here. Game logic is advanced using events gathered
// by the input thread, and rendering blocks on vsync to provide rate control.

#pragma once

#include "sys/window.hpp"

namespace minote {

// Entry point of the game thread. The thread will exit after window.isClosing()
// returns true. Window context must not be active.
void game(Window& window);

}
