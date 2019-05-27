#ifndef MINO_H
#define MINO_H

#include "linmath.h"

#define PIECE_BOX 4
#define MINOS_PER_PIECE 4

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
typedef struct {
	int x, y;
} coord;
typedef coord tetromino[MINOS_PER_PIECE];
typedef tetromino rotationSystem[PieceSize][4];

extern vec4 minoColors[MinoSize];
extern rotationSystem rs;

#endif