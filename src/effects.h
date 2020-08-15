/**
 * Drawing pretty things requested by other layers
 * @file
 */

#ifndef MINOTE_EFFECTS_H
#define MINOTE_EFFECTS_H

#include <stddef.h>
#include "basetypes.h"

/// Particle effect where each instance follows a spiral path
typedef struct EffectSwirl {
	// params
} EffectSwirl;

/**
 * Initialize the effects layer. This must be called before any other effects
 * functions.
 */
void effectsInit(void);

/**
 * Cleanup the effects layer. No other effects function can be used until
 * effectsInit() is called again.
 */
void effectsCleanup(void);

/**
 * Update effects instances to advance their state and remove expired ones.
 */
void effectsUpdate(void);

/**
 * Draw all existing effects instances to the screen.
 */
void effectsDraw(void);

/**
 * Generate ::EffectSwirl particles.
 * @param position World space position of the origin
 * @param count Number of particles
 * @param params Generation parameters
 */
void effectsSwirl(point3f position, size_t count, EffectSwirl* params);

#endif //MINOTE_EFFECTS_H
