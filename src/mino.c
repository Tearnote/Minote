/**
 * Implementation of mino.h
 * @file
 */

#include "mino.h"

#include <assert.h>
#include "util.h"

piece rotationSystem[MinoGarbage][SpinSize] = {
	{}, // MinoNone
	{ // MinoI
		{
			{.x = 0, .y = 2}, {.x = 1, .y = 2},
			{.x = 2, .y = 2}, {.x = 3, .y = 2}
		},
		{
			{.x = 2, .y = 3}, {.x = 2, .y = 2},
			{.x = 2, .y = 1}, {.x = 2, .y = 0}
		},
		{
			{.x = 0, .y = 2}, {.x = 1, .y = 2},
			{.x = 2, .y = 2}, {.x = 3, .y = 2}
		},
		{
			{.x = 2, .y = 3}, {.x = 2, .y = 2},
			{.x = 2, .y = 1}, {.x = 2, .y = 0}
		}
	},
	{ // MinoL
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
			{.x = 1, .y = 2}, {.x = 1, .y = 1},
			{.x = 1, .y = 0}, {.x = 2, .y = 0}
		}
	},
	{ // MinoO
		{
			{.x = 1, .y = 1}, {.x = 2, .y = 1},
			{.x = 1, .y = 0}, {.x = 2, .y = 0}
		},
		{
			{.x = 1, .y = 1}, {.x = 2, .y = 1},
			{.x = 1, .y = 0}, {.x = 2, .y = 0}
		},
		{
			{.x = 1, .y = 1}, {.x = 2, .y = 1},
			{.x = 1, .y = 0}, {.x = 2, .y = 0}
		},
		{
			{.x = 1, .y = 1}, {.x = 2, .y = 1},
			{.x = 1, .y = 0}, {.x = 2, .y = 0}
		}
	},
	{ // MinoZ
		{
			{.x = 0, .y = 1}, {.x = 1, .y = 1},
			{.x = 1, .y = 0}, {.x = 2, .y = 0}
		},
		{
			{.x = 2, .y = 2}, {.x = 1, .y = 1},
			{.x = 2, .y = 1}, {.x = 1, .y = 0}
		},
		{
			{.x = 0, .y = 1}, {.x = 1, .y = 1},
			{.x = 1, .y = 0}, {.x = 2, .y = 0}
		},
		{
			{.x = 2, .y = 2}, {.x = 1, .y = 1},
			{.x = 2, .y = 1}, {.x = 1, .y = 0}
		}
	},
	{ // MinoT
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
			{.x = 1, .y = 2}, {.x = 1, .y = 1},
			{.x = 2, .y = 1}, {.x = 1, .y = 0}
		}
	},
	{ // MinoJ
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
			{.x = 1, .y = 2}, {.x = 2, .y = 2},
			{.x = 1, .y = 1}, {.x = 1, .y = 0}
		}
	},
	{ // MinoS
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

static const color4 MinoColors[MinoSize] = {
	[MinoNone] = Color4Clear,
	[MinoI] = {1.0f, 0.0f, 0.0f, 1.0f},
	[MinoL] = {1.0f, .22f, 0.0f, 1.0f},
	[MinoO] = {1.0f, 1.0f, 0.0f, 1.0f},
	[MinoZ] = {0.0f, 1.0f, 0.0f, 1.0f},
	[MinoT] = {0.0f, 1.0f, 1.0f, 1.0f},
	[MinoJ] = {0.0f, 0.0f, 1.0f, 1.0f},
	[MinoS] = {1.0f, 0.0f, 1.0f, 1.0f},
	[MinoGarbage] = {0.2f, 0.2f, 0.2f, 1.0f}
};

void spinClockwise(spin* val)
{
	if (*val == SpinNone)
		*val = Spin270;
	else
		*val -= 1;
}

void spinCounterClockwise(spin* val)
{
	if (*val == Spin270)
		*val = SpinNone;
	else
		*val += 1;
}

piece* getPiece(mino type, spin rotation)
{
	return &rotationSystem[type][rotation];
}

color4 minoColor(mino type)
{
	assert(type >= MinoNone && type < MinoSize);
	return MinoColors[type];
}

////////////////////////////////////////////////////////////////////////////////

struct Field {
	mino* grid;
	size2i size;
};

Field* fieldCreate(size2i size)
{
	assert(size.x > 0 && size.y > 0);
	Field* result = alloc(sizeof(*result));
	result->size.x = size.x;
	result->size.y = size.y;
	result->grid = alloc(sizeof(mino) * size.x * size.y);
	return result;
}

void fieldDestroy(Field* f)
{
	assert(f);
	free(f->grid);
	f->grid = null;
	free(f);
	f = null;
}

void fieldSet(Field* f, point2i place, mino value)
{
	assert(f);
	assert(value < MinoSize);
	if (place.x < 0 || place.x >= f->size.x) return;
	if (place.y < 0 || place.y >= f->size.y) return;
	f->grid[place.y * f->size.x + place.x] = value;
}

mino fieldGet(Field* f, point2i place)
{
	assert(f);
	if (place.x < 0 || place.x >= f->size.x) return MinoGarbage;
	if (place.y < 0) return MinoGarbage;
	if (place.y >= f->size.y) return MinoNone;
	return f->grid[place.y * f->size.x + place.x];
}

void fieldClearRow(Field* f, int row)
{
	assert(f);
	for (int x = 0; x < f->size.x; x += 1)
		fieldSet(f, (point2i){
			.x = x,
			.y = row
		}, MinoNone);
}

void fieldDropRow(Field* f, int row)
{
	assert(f);
	for (int y = row; y < f->size.y; y += 1) {
		for (int x = 0; x < f->size.x; x += 1) {
			fieldSet(f, (point2i){
				.x = x,
				.y = y
			}, fieldGet(f, (point2i){
				.x = x,
				.y = y + 1
			}));
		}
	}
}

bool fieldIsRowFull(Field* f, int row)
{
	assert(f);
	for (int x = 0; x < f->size.x; x += 1) {
		if (!fieldGet(f, (point2i){
			.x = x,
			.y = row
		}))
			return false;
	}
	return true;
}

bool fieldIsEmpty(Field* f)
{
	assert(f);
	for (int y = 0; y < f->size.y; y += 1) {
		for (int x = 0; x < f->size.x; x += 1) {
			if (fieldGet(f, (point2i){
				.x = x,
				.y = y
			}))
				return false;
		}
	}
	return true;
}

void fieldStampPiece(Field* f, piece* piece, point2i place, mino type)
{
	assert(f);
	assert(piece);
	assert(type < MinoSize);
	for (int i = 0; i < MinosPerPiece; i += 1) {
		fieldSet(f, (point2i){
			.x = place.x + (*piece)[i].x,
			.y = place.y + (*piece)[i].y
		}, type);
	}
}

bool pieceOverlapsField(piece* p, point2i pPos, Field* field)
{
	assert(p);
	assert(field);
	for (size_t i = 0; i < MinosPerPiece; i += 1) {
		point2i cell = {
			.x = (*p)[i].x + pPos.x,
			.y = (*p)[i].y + pPos.y
		};
		if (fieldGet(field, cell))
			return true;
	}
	return false;
}
