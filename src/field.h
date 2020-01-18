/**
 * Object type containing a field of minos
 * @file
 */

#ifndef MINOTE_FIELD_H
#define MINOTE_FIELD_H

#include "visualtypes.h"

typedef enum mino {
	MinoNone,
	MinoI,
	MinoL,
	MinoO,
	MinoZ,
	MinoT,
	MinoJ,
	MinoS,
	MinoGarbage,
	MinoSize
} mino;

typedef struct Field Field;

Field* fieldCreate(size2i size);

void fieldDestroy(Field* f);

void fieldSet(Field* f, point2i place, mino value);

mino fieldGet(Field* f, point2i place);

color4 minoColor(mino type);

#endif //MINOTE_FIELD_H
