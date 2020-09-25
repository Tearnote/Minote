/**
 * Implementation of mrsdef.h
 * @file
 */
 
#include "mrsdef.hpp"

#include "util.hpp"

piece MrsPieces[MinoGarbage] = {
	{}, // MinoNone
	{ // MinoI
		{-1, 0}, {0, 0},
		{1, 0}, {2, 0}
	},
	{ // MinoL
		{-1, 0}, {0, 0},
		{1, 0}, {-1, -1}
	},
	{ // MinoO
		{0, 0}, {1, 0},
		{0, -1}, {1, -1}
	},
	{ // MinoZ
		{-1, 0}, {0, 0},
		{0, -1}, {1, -1},
	},
	{ // MinoT
		{-1, 0}, {0, 0},
		{1, 0}, {0, -1}
	},
	{ // MinoJ
		{-1, 0}, {0, 0},
		{1, 0}, {1, -1}
	},
	{ // MinoS
		{0, 0}, {1, 0},
		{-1, -1}, {0, -1}
	}
};
