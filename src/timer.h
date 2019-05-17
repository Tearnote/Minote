#ifndef TIMER_H
#define TIMER_H

#include <stdint.h>
typedef int64_t nsec;

#define SEC  1000000000
#define MSEC 1000000

void initTimer();
void cleanupTimer();
nsec getTime();
void sleep(nsec ns);

#endif