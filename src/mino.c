#include "mino.h"

#include "linmath.h"

vec4 minoColors[MinoSize] = {
	{   0,   0,   0, 0 }, // MinoNone
	{   1,   0,   0, 1 }, // MinoI
	{   1, 0.5,   0, 1 }, // MinoL
	{   1,   1,   0, 1 }, // MinoO
	{   0,   1,   0, 1 }, // MinoZ
	{   0,   1,   1, 1 }, // MinoT
	{   0,   0,   1, 1 }, // MinoJ
	{   1,   0,   1, 1 }, // MinoS
	{ 0.5, 0.5, 0.5, 1 }, // MinoGarbage
	{   1,   1,   1, 1 }  // MinoPending
};