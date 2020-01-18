/**
 * Object type containing a field of minos
 * @file
 */

#ifndef MINOTE_FIELD_H
#define MINOTE_FIELD_H

#include "visualtypes.h"

/// Types of minos that can appear on the playfield
typedef enum mino {
	MinoNone, ///< zero value
	MinoI, ///< I piece (red)
	MinoL, ///< L piece (orange)
	MinoO, ///< O piece (yellow)
	MinoZ, ///< Z piece (green)
	MinoT, ///< T piece (cyan)
	MinoJ, ///< J piece (blue)
	MinoS, ///< S piece (purple)
	MinoGarbage, ///< mino from any source other than player piece
	MinoSize ///< terminator
} mino;

/// Opaque playfield grid. You can obtain an instance with fieldCreate().
typedef struct Field Field;

/**
 * Create a new ::Field instance.
 * @param size 2D dimensions of the new Field
 * @return A newly created ::Field. Needs to be destroyed with fieldDestroy()
 */
Field* fieldCreate(size2i size);

/**
 * Destroy a ::Field instance. The destroyed object cannot be used anymore and
 * the pointer becomes invalid.
 * @param f The ::Field object
 */
void fieldDestroy(Field* f);

/**
 * Set a single cell of a ::Field to a new ::mino value. Passing coordinates
 * that are out of bounds is safe and a no-op.
 * @param f The ::Field object
 * @param place Coordinate to modify. Can be out of bounds
 * @param value New value to write into the specified coordinate
 */
void fieldSet(Field* f, point2i place, mino value);

/**
 * Retrieve the value of a single cell of a ::Field. Out of bounds coordinates
 * are handled by assuming everything above the field is empty, and everything
 * else is solid.
 * @param f The ::Field object
 * @param place Coordinate to retriee. Can be out of bounds
 * @return Value at the specified coordinate
 */
mino fieldGet(Field* f, point2i place);

/**
 * Return the canonical color of a ::mino.
 * @param type Mino being queried
 * @return The base color of @a type
 */
color4 minoColor(mino type);

#endif //MINOTE_FIELD_H
