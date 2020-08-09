/**
 * Implementation of mrstables.h
 * @file
 */

#include "mrstables.h"

#include "mino.h"

static piece MrsRotations[MinoGarbage][SpinSize] = {
	{}, /* MinoNone */
	{ /* MinoI */
		{
			{.x = 0, .y = 0}, {.x = 1, .y = 0},
			{.x = 2, .y = 0}, {.x = 3, .y = 0}
		},
		{
			{.x = 0, .y = 3}, {.x = 0, .y = 2},
			{.x = 0, .y = 1}, {.x = 0, .y = 0}
		},
		{
			{.x = 0, .y = 0}, {.x = 1, .y = 0},
			{.x = 2, .y = 0}, {.x = 3, .y = 0}
		},
		{
			{.x = 0, .y = 3}, {.x = 0, .y = 2},
			{.x = 0, .y = 1}, {.x = 0, .y = 0}
		}
	},
	{ /* MinoL */
		{
			{.x = 0, .y = 1}, {.x = 1, .y = 1},
			{.x = 2, .y = 1}, {.x = 0, .y = 0}
		},
		{
			{.x = 0, .y = 2}, {.x = 1, .y = 2},
			{.x = 1, .y = 1}, {.x = 1, .y = 0}
		},
		{
			{.x = 2, .y = 1}, {.x = 0, .y = 0},
			{.x = 1, .y = 0}, {.x = 2, .y = 0}
		},
		{
			{.x = 0, .y = 2}, {.x = 0, .y = 1},
			{.x = 0, .y = 0}, {.x = 1, .y = 0}
		}
	},
	{ /* MinoO */
		{
			{.x = 0, .y = 1}, {.x = 1, .y = 1},
			{.x = 0, .y = 0}, {.x = 1, .y = 0}
		},
		{
			{.x = 0, .y = 1}, {.x = 1, .y = 1},
			{.x = 0, .y = 0}, {.x = 1, .y = 0}
		},
		{
			{.x = 0, .y = 1}, {.x = 1, .y = 1},
			{.x = 0, .y = 0}, {.x = 1, .y = 0}
		},
		{
			{.x = 0, .y = 1}, {.x = 1, .y = 1},
			{.x = 0, .y = 0}, {.x = 1, .y = 0}
		}
	},
	{ /* MinoZ */
		{
			{.x = 0, .y = 1}, {.x = 1, .y = 1},
			{.x = 1, .y = 0}, {.x = 2, .y = 0}
		},
		{
			{.x = 1, .y = 2}, {.x = 0, .y = 1},
			{.x = 1, .y = 1}, {.x = 0, .y = 0}
		},
		{
			{.x = 0, .y = 1}, {.x = 1, .y = 1},
			{.x = 1, .y = 0}, {.x = 2, .y = 0}
		},
		{
			{.x = 1, .y = 2}, {.x = 0, .y = 1},
			{.x = 1, .y = 1}, {.x = 0, .y = 0}
		}
	},
	{ /* MinoT */
		{
			{.x = 0, .y = 1}, {.x = 1, .y = 1},
			{.x = 2, .y = 1}, {.x = 1, .y = 0}
		},
		{
			{.x = 1, .y = 2}, {.x = 0, .y = 1},
			{.x = 1, .y = 1}, {.x = 1, .y = 0}
		},
		{
			{.x = 1, .y = 1}, {.x = 0, .y = 0},
			{.x = 1, .y = 0}, {.x = 2, .y = 0}
		},
		{
			{.x = 0, .y = 2}, {.x = 0, .y = 1},
			{.x = 1, .y = 1}, {.x = 0, .y = 0}
		}
	},
	{ /* MinoJ */
		{
			{.x = 0, .y = 1}, {.x = 1, .y = 1},
			{.x = 2, .y = 1}, {.x = 2, .y = 0}
		},
		{
			{.x = 1, .y = 2}, {.x = 1, .y = 1},
			{.x = 0, .y = 0}, {.x = 1, .y = 0}
		},
		{
			{.x = 0, .y = 1}, {.x = 0, .y = 0},
			{.x = 1, .y = 0}, {.x = 2, .y = 0}
		},
		{
			{.x = 0, .y = 2}, {.x = 1, .y = 2},
			{.x = 0, .y = 1}, {.x = 0, .y = 0}
		}
	},
	{ /* MinoS */
		{
			{.x = 1, .y = 1}, {.x = 2, .y = 1},
			{.x = 0, .y = 0}, {.x = 1, .y = 0}
		},
		{
			{.x = 0, .y = 2}, {.x = 0, .y = 1},
			{.x = 1, .y = 1}, {.x = 1, .y = 0}
		},
		{
			{.x = 1, .y = 1}, {.x = 2, .y = 1},
			{.x = 0, .y = 0}, {.x = 1, .y = 0}
		},
		{
			{.x = 0, .y = 2}, {.x = 0, .y = 1},
			{.x = 1, .y = 1}, {.x = 1, .y = 0}
		}
	}
};

// I know most of this data is redundant, it's just easier to use this way
static MrsPoint MrsOffsets[MinoGarbage][SpinSize] = {
	{}, /* MinoNone */
	{ /* MinoI */
		{.x = 0, .xHalf = 0, .y = 1, .yHalf = 1},
		{.x = 1, .xHalf = 1, .y = 0, .yHalf = 0},
		{.x = 0, .xHalf = 0, .y = 1, .yHalf = 1},
		{.x = 1, .xHalf = 1, .y = 0, .yHalf = 0}
	},
	{ /* MinoL */
		{.x = 0, .xHalf = 1, .y = 1, .yHalf = 0},
		{.x = 1, .xHalf = 0, .y = 0, .yHalf = 1},
		{.x = 0, .xHalf = 1, .y = 1, .yHalf = 0},
		{.x = 1, .xHalf = 0, .y = 0, .yHalf = 1}
	},
	{ /* MinoO */
		{.x = 1, .xHalf = 0, .y = 1, .yHalf = 0},
		{.x = 1, .xHalf = 0, .y = 1, .yHalf = 0},
		{.x = 1, .xHalf = 0, .y = 1, .yHalf = 0},
		{.x = 1, .xHalf = 0, .y = 1, .yHalf = 0}
	},
	{ /* MinoZ */
		{.x = 0, .xHalf = 1, .y = 1, .yHalf = 0},
		{.x = 1, .xHalf = 0, .y = 0, .yHalf = 1},
		{.x = 0, .xHalf = 1, .y = 1, .yHalf = 0},
		{.x = 1, .xHalf = 0, .y = 0, .yHalf = 1}
	},
	{ /* MinoT */
		{.x = 0, .xHalf = 1, .y = 1, .yHalf = 0},
		{.x = 1, .xHalf = 0, .y = 0, .yHalf = 1},
		{.x = 0, .xHalf = 1, .y = 1, .yHalf = 0},
		{.x = 1, .xHalf = 0, .y = 0, .yHalf = 1}
	},
	{ /* MinoJ */
		{.x = 0, .xHalf = 1, .y = 1, .yHalf = 0},
		{.x = 1, .xHalf = 0, .y = 0, .yHalf = 1},
		{.x = 0, .xHalf = 1, .y = 1, .yHalf = 0},
		{.x = 1, .xHalf = 0, .y = 0, .yHalf = 1}
	},
	{ /* MinoS */
		{.x = 0, .xHalf = 1, .y = 1, .yHalf = 0},
		{.x = 1, .xHalf = 0, .y = 0, .yHalf = 1},
		{.x = 0, .xHalf = 1, .y = 1, .yHalf = 0},
		{.x = 1, .xHalf = 0, .y = 0, .yHalf = 1}
	}
};

piece* mrsGetPiece(mino type, spin rotation)
{
	return &MrsRotations[type][rotation];
}

MrsPoint mrsGetPieceOffset(mino type, spin rotation)
{
	return MrsOffsets[type][rotation];
}
