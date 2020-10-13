/**
 * Drawing pretty things requested by other layers
 * @file
 */

#ifndef MINOTE_PARTICLES_H
#define MINOTE_PARTICLES_H

#include <stddef.h>
#include "base/math.hpp"
#include "base/tween.hpp"
#include "base/ease.hpp"
#include "base/time.hpp"

/// Details of a particle effect
typedef struct ParticleParams {
	minote::color4 color; ///< Tint of every particle
	minote::nsec durationMin; ///< Smallest possible duration
	minote::nsec durationMax; ///< Largest possible duration
	float distanceMin; ///< Smallest distance travelled
	float distanceMax; ///< Largest distance travelled
	float spinMin; ///< Smallest rate of turning
	float spinMax; ///< Largest rate of turning
	int directionVert; ///< 1 for up, -1 for down, 0 for random
	int directionHorz; ///< 1 for right, -1 for left, 0 for random
	minote::EasingFunction<float> ease; ///< Overall easing profile of the particles' path
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
 * Update active particles to remove expired ones.
 */
void particlesUpdate(void);

/**
 * Draw all active particles to the screen at their current position.
 */
void particlesDraw(void);

/**
 * Generate particles with specific parameters.
 * @param position World space position of the origin
 * @param count Number of particles
 * @param params Generation parameters
 */
void particlesGenerate(minote::vec3 position, size_t count, ParticleParams* params);

#endif //MINOTE_PARTICLES_H
