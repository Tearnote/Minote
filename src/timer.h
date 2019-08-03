// Minote - timer.h
// Timing utilities and wrappers
// All time is nanosecond based for maximum resolution without floats

#ifndef TIMER_H
#define TIMER_H

#include <stdint.h>

// Maximum value of 292.3 years, please do not run the game for longer than that
typedef int64_t nsec;

#define SEC  1000000000 // Nanoseconds in a second
#define MSEC 1000000 // Nanoseconds in a millisecond

void initTimer(void);
void cleanupTimer(void);
nsec getTime(void);
// Resolution is OS-dependent, ~1.5ms at worst (on Windows, obviously)
void sleep(nsec ns);

#endif // TIMER_H
