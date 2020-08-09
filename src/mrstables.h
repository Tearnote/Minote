/**
 * Various table data for the mrs sublayer
 * @file
 */

#ifndef MINOTE_MRSTABLES_H
#define MINOTE_MRSTABLES_H

#include "mino.h"

/// Position of the player piece in MRS. Can be safely cast to ::point2i.
typedef struct MrsPoint {
	struct {
		int x; ///< Integer coordinate
		int y; ///< Integer coordinate
	};
	bool xHalf; ///< true if 0.5 shift, false if no shift
	bool yHalf; ///< true if 0.5 shift, false if no shift
} MrsPoint;

/**
 * Query the rotation system for a specific piece. This info needs
 * to be combined with offsets from getPieceOffset().
 * @param type Type of the piece, between MinoNone and MinoGarbage (exclusive)
 * @param rotation Spin of the piece
 * @return Read-only piece data
 */
piece* mrsGetPiece(mino type, spin rotation);

/**
 * Query the rotation system for a specific piece's offset. The offset
 * should be added to positions received from getPiece() in order to get
 * the correct position of the piece.
 * @param type Type of the piece, between MinoNone and MinoGarbage (exclusive)
 * @param rotation Spin of the piece
 * @return Read-only offset data
 */
MrsPoint mrsGetPieceOffset(mino type, spin rotation);

#endif //MINOTE_MRSTABLES_H
