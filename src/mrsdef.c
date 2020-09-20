/**
 * Implementation of mrsdef.h
 * @file
 */
 
#include "mrsdef.h"

#include "util.h"

piece MrsPieces[MinoGarbage] = {
	{}, // MinoNone
	{ // MinoI
		{.x = -1, .y = 0}, {.x = 0, .y = 0},
		{.x = 1, .y = 0}, {.x = 2, .y = 0}
	},
	{ // MinoL
		{.x = -1, .y = 0}, {.x = 0, .y = 0},
		{.x = 1, .y = 0}, {.x = -1, .y = -1}
	},
	{ // MinoO
		{.x = 0, .y = 0}, {.x = 1, .y = 0},
		{.x = 0, .y = -1}, {.x = 1, .y = -1}
	},
	{ // MinoZ
		{.x = -1, .y = 0}, {.x = 0, .y = 0},
		{.x = 0, .y = -1}, {.x = 1, .y = -1},
	},
	{ // MinoT
		{.x = -1, .y = 0}, {.x = 0, .y = 0},
		{.x = 1, .y = 0}, {.x = 0, .y = -1}
	},
	{ // MinoJ
		{.x = -1, .y = 0}, {.x = 0, .y = 0},
		{.x = 1, .y = 0}, {.x = 1, .y = -1}
	},
	{ // MinoS
		{.x = 0, .y = 0}, {.x = 1, .y = 0},
		{.x = -1, .y = -1}, {.x = 0, .y = -1}
	}
};
