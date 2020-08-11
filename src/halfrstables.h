/**
 * Various table data for the halfrs sublayer
 * @file
 */

#ifndef MINOTE_HALFRSTABLES_H
#define MINOTE_HALFRSTABLES_H

#include "mino.h"

/// Position of the player piece in MRS. Can be safely cast to ::point2i.
typedef struct HalfrsPoint {
	struct {
		int x; ///< Integer coordinate
		int y; ///< Integer coordinate
	};
	bool xHalf; ///< true if 0.5 shift, false if no shift
	bool yHalf; ///< true if 0.5 shift, false if no shift
} HalfrsPoint;

/**
 * Query the rotation system for a specific piece. This info needs
 * to be combined with offsets from getPieceOffset().
 * @param type Type of the piece, between MinoNone and MinoGarbage (exclusive)
 * @param rotation Spin of the piece
 * @return Read-only piece data
 */
piece* halfrsGetPiece(mino type, spin rotation);

/**
 * Query the rotation system for a specific piece's offset. The offset
 * should be added to positions received from getPiece() in order to get
 * the correct position of the piece.
 * @param type Type of the piece, between MinoNone and MinoGarbage (exclusive)
 * @param rotation Spin of the piece
 * @return Read-only offset data
 */
HalfrsPoint halfrsGetPieceOffset(mino type, spin rotation);

#endif //MINOTE_HALFRSTABLES_H
