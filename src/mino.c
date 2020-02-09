/**
 * Implementation of field.h
 * @file
 */

#include "mino.h"

#include <assert.h>
#include "util.h"

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

color4 minoColor(mino type)
{
	assert(type >= MinoNone && type < MinoSize);
	return MinoColors[type];
}
