/**
 * Sublayer: play -> purers
 * @file
 * Simulation of Grand Master rotation system.
 */

#ifndef MINOTE_PURERS_H
#define MINOTE_PURERS_H

#include "darray.h"

/// Frequency of game logic updates, simulated by semi-threading, in Hz
#define PurersUpdateFrequency 59.84
/// Inverse of #PurersUpdateFrequency, in ::nsec
#define PurersUpdateTick (secToNsec(1) / PurersUpdateFrequency)

/**
 * Initialize the purers sublayer. Needs to be called before the layer can be
 * used.
 */
void purersInit(void);

/**
 * Clean up the purers sublayer. Play functions cannot be used until playInit() is
 * called again.
 */
void purersCleanup(void);

/**
 * Simulate one frame of gameplay logic.
 * @param in List of ::Input events that happened during the frame
 */
void purersAdvance(darray* inputs);

/**
 * Draw the purers sublayer to the screen.
 */
void purersDraw(void);

#endif //MINOTE_PURERS_H
