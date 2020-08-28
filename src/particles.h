/**
 * Drawing pretty things requested by other layers
 * @file
 */

#ifndef MINOTE_PARTICLES_H
#define MINOTE_PARTICLES_H

#include <stddef.h>
#include "basetypes.h"

/// Details of a particle effect
typedef struct ParticleParams {
	color4 color;
} ParticleParams;

/**
 * Initialize the particles layer. This must be called before any other
 * particles functions.
 */
void particlesInit(void);

/**
 * Cleanup the particles layer. No other particles function can be used until
 * particlesInit() is called again.
 */
void particlesCleanup(void);

/**
 * Update active particles to advance their state and remove expired ones.
 */
void particlesUpdate(void);

/**
 * Draw all active particles to the screen.
 */
void particlesDraw(void);

/**
 * Generate particles with specific parameters.
 * @param position World space position of the origin
 * @param count Number of particles
 * @param params Generation parameters
 */
void particlesGenerate(point3f position, size_t count, ParticleParams* params);

#endif //MINOTE_PARTICLES_H
