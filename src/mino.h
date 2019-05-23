#ifndef MINO_H
#define MINO_H

#include "linmath.h"

typedef enum {
	MinoNone    = 0,
	MinoI       = 1,
	MinoL       = 2,
	MinoO       = 3,
	MinoZ       = 4,
	MinoT       = 5,
	MinoJ       = 6,
	MinoS       = 7,
	MinoGarbage = 8,
	MinoPending = 9,
	MinoSize    = 10 // Convenience value, not a real mino
} mino;

extern vec4 minoColors[MinoSize];

#endif