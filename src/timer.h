// Minote - timer.h

#ifndef MINOTE_TIMER_H
#define MINOTE_TIMER_H

#include <cstdint>

using nsec = int64_t;

inline auto secToNsec(double sec) -> nsec
{
	return sec * 1'000'000'000;
}

#endif //MINOTE_TIMER_H
