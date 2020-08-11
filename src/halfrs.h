/**
 * Sublayer: play -> halfrs
 * @file
 * An attempt at a rotation system where pieces rotate around the exact center
 * of their bounding box.
 */

#ifndef MINOTE_HALFRS_H
#define MINOTE_HALFRS_H

#include "darray.h"

/// Frequency of game logic updates, simulated by semi-threading, in Hz
#define HalfrsUpdateFrequency 60.0
/// Inverse of #PureUpdateFrequency, in ::nsec
#define HalfrsUpdateTick (secToNsec(1) / HalfrsUpdateFrequency)

/**
 * Initialize the halfrs sublayer. Needs to be called before the layer can be
 * used.
 */
void halfrsInit(void);

/**
 * Clean up the halfrs sublayer. Play functions cannot be used until playInit() is
 * called again.
 */
void halfrsCleanup(void);

/**
 * Simulate one frame of gameplay logic.
 * @param in List of ::Input events that happened during the frame
 */
void halfrsAdvance(darray* inputs);

/**
 * Draw the halfrs sublayer to the screen.
 */
void halfrsDraw(void);

#endif //MINOTE_HALFRS_H
