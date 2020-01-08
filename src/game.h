/**
 * Simulation and rendering thread (game thread)
 * @file
 * Most of the work is done here. Simulation is advanced using inputs gathered
 * by the input thread, and rendering blocks on vsync to provide rate control.
 */

#ifndef MINOTE_GAME_H
#define MINOTE_GAME_H

/**
 * Entry point of the game thread. Call spawnThread(game) only after all systems
 * other than renderer are initialized. The thread will exit when
 * windowIsOpen() returns false.
 * @param arg Unused
 * @return Unused, always null
 */
void* game(void* arg);

#endif //MINOTE_GAME_H
