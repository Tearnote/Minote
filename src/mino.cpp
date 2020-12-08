/**
 * Implementation of mino.h
 * @file
 */

#include "mino.hpp"

#include "base/util.hpp"

using namespace minote;

static const color4 MinoColors[MinoSize] = {
	[MinoNone] = {1.0f, 1.0f, 1.0f, 0.0f},
	[MinoI] = {1.0f, 0.0f, 0.0f, 1.0f},
	[MinoL] = {1.0f, .22f, 0.0f, 1.0f},
	[MinoO] = {1.0f, 1.0f, 0.0f, 1.0f},
	[MinoZ] = {0.0f, 1.0f, 0.0f, 1.0f},
	[MinoT] = {0.0f, 1.0f, 1.0f, 1.0f},
	[MinoJ] = {0.0f, 0.0f, 1.0f, 1.0f},
	[MinoS] = {1.0f, 0.0f, 1.0f, 1.0f},
	[MinoGarbage] = {.22f, .22f, .22f, 1.0f}
};

void spinClockwise(spin* val)
{
	if (*val == SpinNone)
		*val = Spin270;
	else
		*val = static_cast<spin>(*val - 1);
}

void spinCounterClockwise(spin* val)
{
	if (*val == Spin270)
		*val = SpinNone;
	else
		*val = static_cast<spin>(*val + 1);
}

color4 minoColor(mino type)
{
	ASSERT(type >= MinoNone && type < MinoSize);
	return MinoColors[type];
}

void pieceRotate(piece p, spin rotation)
{
	ASSERT(p);
	ASSERT(rotation < SpinSize);

	for (int i = 0; i < rotation; i += 1) { // Repeat as many times as rotations
		for (size_t j = 0; j < MinosPerPiece; j += 1) { // Rotate each mino
			ivec2 newPos{
				-p[j].y,
				p[j].x
			};
			p[j] = newPos;
		}
	}
}

////////////////////////////////////////////////////////////////////////////////

Field* fieldCreate(ivec2 size)
{
	ASSERT(size.x > 0 && size.y > 0);
	auto* result = allocate<Field>();
	result->size.x = size.x;
	result->size.y = size.y;
	result->grid = allocate<mino>(size.x * size.y);
	return result;
}

void fieldDestroy(Field* f)
{
	ASSERT(f);
	free(f->grid);
	f->grid = nullptr;
	free(f);
	f = nullptr;
}

void fieldSet(Field* f, ivec2 place, mino value)
{
	ASSERT(f);
	ASSERT(value < MinoSize);
	if (place.x < 0 || place.x >= f->size.x) return;
	if (place.y < 0 || place.y >= f->size.y) return;
	f->grid[place.y * f->size.x + place.x] = value;
}

mino fieldGet(Field* f, ivec2 place)
{
	ASSERT(f);
	if (place.x < 0 || place.x >= f->size.x) return MinoGarbage;
	if (place.y < 0) return MinoGarbage;
	if (place.y >= f->size.y) return MinoNone;
	return f->grid[place.y * f->size.x + place.x];
}

void fieldClearRow(Field* f, int row)
{
	ASSERT(f);
	for (int x = 0; x < f->size.x; x += 1)
		fieldSet(f, (ivec2){
			x,
			row
		}, MinoNone);
}

void fieldDropRow(Field* f, int row)
{
	ASSERT(f);
	for (int y = row; y < f->size.y; y += 1) {
		for (int x = 0; x < f->size.x; x += 1) {
			fieldSet(f, (ivec2){
				x,
				y
			}, fieldGet(f, (ivec2){
				x,
				y + 1
			}));
		}
	}
}

bool fieldIsRowFull(Field* f, int row)
{
	ASSERT(f);
	for (int x = 0; x < f->size.x; x += 1) {
		if (!fieldGet(f, (ivec2){
			x,
			row
		}))
			return false;
	}
	return true;
}

bool fieldIsEmpty(Field* f)
{
	ASSERT(f);
	for (int y = 0; y < f->size.y; y += 1) {
		for (int x = 0; x < f->size.x; x += 1) {
			if (fieldGet(f, (ivec2){
				x,
				y
			}))
				return false;
		}
	}
	return true;
}

void fieldStampPiece(Field* f, piece* piece, ivec2 place, mino type)
{
	ASSERT(f);
	ASSERT(piece);
	ASSERT(type < MinoSize);
	for (int i = 0; i < MinosPerPiece; i += 1) {
		fieldSet(f, (ivec2){
			place.x + (*piece)[i].x,
			place.y + (*piece)[i].y
		}, type);
	}
}

bool pieceOverlapsField(piece* p, ivec2 pPos, Field* field)
{
	ASSERT(p);
	ASSERT(field);
	for (size_t i = 0; i < MinosPerPiece; i += 1) {
		ivec2 cell = {
			(*p)[i].x + pPos.x,
			(*p)[i].y + pPos.y
		};
		if (fieldGet(field, cell))
			return true;
	}
	return false;
}
