/**
 * Definitions and objects for dealing with minos, pieces and fields
 * @file
 */

#ifndef MINOTE_MINO_H
#define MINOTE_MINO_H

#include <stdbool.h>
#include "basetypes.h"

/**
 * Possible states of a ::Field cell. Values below #MinoGarbage are also valid
 * player pieces.
 */
typedef enum mino {
	MinoNone, ///< zero value
	MinoI, ///< red
	MinoL, ///< orange
	MinoO, ///< yellow
	MinoZ, ///< green
	MinoT, ///< cyan
	MinoJ, ///< blue
	MinoS, ///< purple
	MinoGarbage, ///< mino from any source other than player piece
	MinoSize ///< terminator
} mino;

/// Orthogonal rotation, increasing counterclockwise
typedef enum spin {
	SpinNone, ///< 0 degrees
	Spin90,
	Spin180,
	Spin270,
	SpinSize ///< terminator
} spin;

#define MinosPerPiece 4
#define CenterColumn 1 ///< Used in kick exception rules

/// Shape of a player piece at a specific ::spin
typedef point2i piece[MinosPerPiece];

/**
 * Rotate a spin to the next clockwise value.
 * @param[in,out] val Address of the ::spin to alter
 */
void spinClockwise(spin* val);

/**
 * Rotate a spin to the next counter-clockwise value.
 * @param val Address of the ::spin to alter
 */
void spinCounterClockwise(spin* val);

/**
 * Query the rotation system for a specific piece.
 * @param type Type of the piece, between MinoNone and MinoGarbage (exclusive)
 * @param rotation Spin of the piece
 * @return Read-only piece data
 */
piece* getPiece(mino type, spin rotation);

/**
 * Return the canonical color of a ::mino.
 * @param type Mino being queried
 * @return The base color of @a type
 */
color4 minoColor(mino type);

////////////////////////////////////////////////////////////////////////////////

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
 * Check if a piece is on top of any taken cells of the field.
 * @param p Piece to test
 * @param pPos Offset of @a p on the @a field
 * @param field Field to test
 * @return true if there is overlap, false if there is not
 */
bool pieceOverlapsField(piece* p, point2i pPos, Field* field);

#endif //MINOTE_MINO_H
