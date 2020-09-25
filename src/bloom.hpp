/**
 * Post-processing filter for adding light bleed around HDR pixels.
 * @file
 */

#ifndef MINOTE_BLOOM_H
#define MINOTE_BLOOM_H

/**
 * Initialize the bloom filter. Must be called after rendererInit().
 */
void bloomInit(void);

/**
 * Cleanup the bloom filter. No other bloom functions can be used until
 * bloomInit() is called again.
 */
void bloomCleanup(void);

/**
 * Apply the bloom effect.
 */
void bloomApply(void);

#endif //MINOTE_BLOOM_H
