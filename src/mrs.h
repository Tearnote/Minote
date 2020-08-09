/**
 * Sublayer: play -> mrs
 * @file
 * The original Minote rotation system.
 */

#ifndef MINOTE_MRS_H
#define MINOTE_MRS_H

#include "darray.h"

/// Frequency of game logic updates, simulated by semi-threading, in Hz
#define MrsUpdateFrequency 60.0
/// Inverse of #PureUpdateFrequency, in ::nsec
#define MrsUpdateTick (secToNsec(1) / MrsUpdateFrequency)

/**
 * Initialize the mrs sublayer. Needs to be called before the layer can be
 * used.
 */
void mrsInit(void);

/**
 * Clean up the mrs sublayer. Play functions cannot be used until playInit() is
 * called again.
 */
void mrsCleanup(void);

/**
 * Simulate one frame of gameplay logic.
 * @param in List of ::Input events that happened during the frame
 */
void mrsAdvance(darray* inputs);

/**
 * Draw the mrs sublayer to the screen.
 */
void mrsDraw(void);

#endif //MINOTE_MRS_H
