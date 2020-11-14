/**
 * Post-processing filter for adding light bleed around HDR pixels.
 * @file
 */

#ifndef MINOTE_BLOOM_H
#define MINOTE_BLOOM_H

#include "sys/window.hpp"
#include "engine/engine.hpp"

/**
 * Initialize the bloom filter. Must be called after rendererInit().
 */
void bloomInit(minote::Window& window);

/**
 * Cleanup the bloom filter. No other bloom functions can be used until
 * bloomInit() is called again.
 */
void bloomCleanup(void);

/**
 * Apply the bloom effect.
 */
void bloomApply(minote::Engine& engine);

#endif //MINOTE_BLOOM_H
