/**
 * Various table data for the mrs sublayer
 * @file
 */

#ifndef MINOTE_MRSTABLES_H
#define MINOTE_MRSTABLES_H

#include "mino.h"

/**
 * Query the rotation system for a specific piece.
 * @param type Type of the piece, between MinoNone and MinoGarbage (exclusive)
 * @param rotation Spin of the piece
 * @return Read-only piece data
 */
piece* mrsGetPiece(mino type, spin rotation);

#endif //MINOTE_MRSTABLES_H
