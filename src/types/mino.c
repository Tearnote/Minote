// Minote - types/mino.c

#include "types/mino.h"

#include "linmath/linmath.h"

vec4 minoColors[MinoSize] = {
	{ 0,    0,    0,    0 }, // MinoNone
	{ 1,    0,    0,    1 }, // MinoI
	{ 1,    .22f, 0,    1 }, // MinoL
	{ 1,    1,    0,    1 }, // MinoO
	{ 0,    1,    0,    1 }, // MinoZ
	{ 0,    1,    1,    1 }, // MinoT
	{ 0,    0,    1,    1 }, // MinoJ
	{ 1,    0,    1,    1 }, // MinoS
	{ .22f, .22f, .22f, 1 }  // MinoGarbage
};

rotationSystem rs = {
	{}, // PieceNone
	{ // PieceI
		{
			{ .x = 0, .y = 1 }, { .x = 1, .y = 1 },
			{ .x = 2, .y = 1 }, { .x = 3, .y = 1 }
		},
		{
			{ .x = 2, .y = 0 }, { .x = 2, .y = 1 },
			{ .x = 2, .y = 2 }, { .x = 2, .y = 3 }
		},
		{
			{ .x = 0, .y = 1 }, { .x = 1, .y = 1 },
			{ .x = 2, .y = 1 }, { .x = 3, .y = 1 }
		},
		{
			{ .x = 2, .y = 0 }, { .x = 2, .y = 1 },
			{ .x = 2, .y = 2 }, { .x = 2, .y = 3 }
		}
	},
	{ // PieceL
		{
			{ .x = 0, .y = 2 }, { .x = 1, .y = 2 },
			{ .x = 2, .y = 2 }, { .x = 0, .y = 3 }
		},
		{
			{ .x = 0, .y = 1 }, { .x = 1, .y = 1 },
			{ .x = 1, .y = 2 }, { .x = 1, .y = 3 }
		},
		{
			{ .x = 2, .y = 2 }, { .x = 0, .y = 3 },
			{ .x = 1, .y = 3 }, { .x = 2, .y = 3 }
		},
		{
			{ .x = 1, .y = 1 }, { .x = 1, .y = 2 },
			{ .x = 1, .y = 3 }, { .x = 2, .y = 3 }
		}
	},
	{ // PieceO
		{
			{ .x = 1, .y = 2 }, { .x = 2, .y = 2 },
			{ .x = 1, .y = 3 }, { .x = 2, .y = 3 }
		},
		{
			{ .x = 1, .y = 2 }, { .x = 2, .y = 2 },
			{ .x = 1, .y = 3 }, { .x = 2, .y = 3 }
		},
		{
			{ .x = 1, .y = 2 }, { .x = 2, .y = 2 },
			{ .x = 1, .y = 3 }, { .x = 2, .y = 3 }
		},
		{
			{ .x = 1, .y = 2 }, { .x = 2, .y = 2 },
			{ .x = 1, .y = 3 }, { .x = 2, .y = 3 }
		}
	},
	{ // PieceZ
		{
			{ .x = 0, .y = 2 }, { .x = 1, .y = 2 },
			{ .x = 1, .y = 3 }, { .x = 2, .y = 3 }
		},
		{
			{ .x = 2, .y = 1 }, { .x = 1, .y = 2 },
			{ .x = 2, .y = 2 }, { .x = 1, .y = 3 }
		},
		{
			{ .x = 0, .y = 2 }, { .x = 1, .y = 2 },
			{ .x = 1, .y = 3 }, { .x = 2, .y = 3 }
		},
		{
			{ .x = 2, .y = 1 }, { .x = 1, .y = 2 },
			{ .x = 2, .y = 2 }, { .x = 1, .y = 3 }
		}
	},
	{ // PieceT
		{
			{ .x = 0, .y = 2 }, { .x = 1, .y = 2 },
			{ .x = 2, .y = 2 }, { .x = 1, .y = 3 }
		},
		{
			{ .x = 1, .y = 1 }, { .x = 0, .y = 2 },
			{ .x = 1, .y = 2 }, { .x = 1, .y = 3 }
		},
		{
			{ .x = 1, .y = 2 }, { .x = 0, .y = 3 },
			{ .x = 1, .y = 3 }, { .x = 2, .y = 3 }
		},
		{
			{ .x = 1, .y = 1 }, { .x = 1, .y = 2 },
			{ .x = 2, .y = 2 }, { .x = 1, .y = 3 }
		}
	},
	{ // PieceJ
		{
			{ .x = 0, .y = 2 }, { .x = 1, .y = 2 },
			{ .x = 2, .y = 2 }, { .x = 2, .y = 3 }
		},
		{
			{ .x = 1, .y = 1 }, { .x = 1, .y = 2 },
			{ .x = 0, .y = 3 }, { .x = 1, .y = 3 }
		},
		{
			{ .x = 0, .y = 2 }, { .x = 0, .y = 3 },
			{ .x = 1, .y = 3 }, { .x = 2, .y = 3 }
		},
		{
			{ .x = 1, .y = 1 }, { .x = 2, .y = 1 },
			{ .x = 1, .y = 2 }, { .x = 1, .y = 3 }
		}
	},
	{ // PieceS
		{
			{ .x = 1, .y = 2 }, { .x = 2, .y = 2 },
			{ .x = 0, .y = 3 }, { .x = 1, .y = 3 }
		},
		{
			{ .x = 0, .y = 1 }, { .x = 0, .y = 2 },
			{ .x = 1, .y = 2 }, { .x = 1, .y = 3 }
		},
		{
			{ .x = 1, .y = 2 }, { .x = 2, .y = 2 },
			{ .x = 0, .y = 3 }, { .x = 1, .y = 3 }
		},
		{
			{ .x = 0, .y = 1 }, { .x = 0, .y = 2 },
			{ .x = 1, .y = 2 }, { .x = 1, .y = 3 }
		}
	}
};

enum mino
getPlayfieldGrid(enum mino field[PLAYFIELD_H][PLAYFIELD_W], int x, int y)
{
	if (x < 0 || x >= PLAYFIELD_W || y >= PLAYFIELD_H)
		return MinoGarbage;
	if (y < 0)
		return MinoNone;
	return field[y][x];
}

void setPlayfieldGrid(enum mino field[PLAYFIELD_H][PLAYFIELD_W],
                      int x, int y, enum mino val)
{
	if (x < 0 || x >= PLAYFIELD_W ||
	    y < 0 || y >= PLAYFIELD_H)
		return;
	field[y][x] = val;
}
