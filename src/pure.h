/**
 * Sublayer: play -> pure
 * @file
 * Simulation of the Pure gamemode.
 */

#ifndef MINOTE_PURE_H
#define MINOTE_PURE_H

#include "darray.h"

/**
 * Initialize the pure sublayer. Needs to be called before the layer can be
 * used.
 */
void pureInit(void);

/**
 * Clean up the pure sublayer. Play functions cannot be used until playInit() is
 * called again.
 */
void pureCleanup(void);

/**
 * Simulate one frame of gameplay logic.
 * @param in List of ::Input events that happened during the frame
 */
void pureAdvance(darray* inputs);

/**
 * Draw the pure sublayer to the screen.
 */
void pureDraw(void);

#endif //MINOTE_PURE_H
