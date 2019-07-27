// Minote - timer.h
// Timing utilities and wrappers
// All time is nanosecond based for maximum resolution without floats

#ifndef TIMER_H
#define TIMER_H

#include <stdint.h>
typedef int64_t nsec; // Maximum value of 292.3 years, please do not run the game for longer than that

#define SEC  1000000000 // Nanoseconds in a second
#define MSEC 1000000 // Nanoseconds in a millisecond

void initTimer(void);
void cleanupTimer(void);
nsec getTime(void);
void sleep(nsec ns); // Resolution is OS-dependent, ~1.5ms at worst (on Windows, obviously)

#endif