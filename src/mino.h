#ifndef MINO_H
#define MINO_H

#include "linmath/linmath.h"

#define PIECE_BOX 4
#define MINOS_PER_PIECE 4

typedef enum {
	MinoI,
	MinoL,
	MinoO,
	MinoZ,
	MinoT,
	MinoJ,
	MinoS,
	MinoGarbage,
	MinoPending,
	MinoNone,
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
typedef struct {
	int x, y;
} coord;
typedef coord piece[MINOS_PER_PIECE];
typedef piece rotationSystem[PieceSize][4];

extern vec4 minoColors[MinoSize];
extern rotationSystem rs;

int rightmostMino(piece p);
int leftmostMino(piece p);

#endif