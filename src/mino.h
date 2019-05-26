#ifndef MINO_H
#define MINO_H

#include "linmath.h"

#define PIECE_BOX 4

typedef enum {
	MinoNone,
	MinoI,
	MinoL,
	MinoO,
	MinoZ,
	MinoT,
	MinoJ,
	MinoS,
	MinoGarbage,
	MinoPending,
	MinoSize
} mino;
typedef enum {
	PieceI,
	PieceL,
	PieceO,
	PieceZ,
	PieceT,
	PieceJ,
	PieceS,
	PieceSize
} pieceType;
typedef mino piece[PIECE_BOX][PIECE_BOX];
typedef piece rotationSystem[PieceSize][4];

extern vec4 minoColors[MinoSize];
extern rotationSystem rs;

#endif