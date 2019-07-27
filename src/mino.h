// Minote - mino.h
// Data structures to handle minos (single blocks) and pieces (groups of offsets)

#ifndef MINO_H
#define MINO_H

#include "linmath/linmath.h"

#define PIECE_BOX 4 // Size of the bounding box all pieces fit into, convenience value
#define MINOS_PER_PIECE 4

// All types of minos that can exist on the playfield
// In addition to minos the player can control, some extra types are reserved for later use
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

// All pieces the player can control
// Values match up with mino enum
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

// Coordinate struct for use with the playfield
typedef struct {
	int x, y;
} coord;

// Pieces themselves do not contain minos or colors
typedef coord piece[MINOS_PER_PIECE];

// All rotation states of every piece
// [pieceType][0] is the spawn rotation
// Advances clockwise
typedef piece rotationSystem[PieceSize][4];

extern vec4 minoColors[MinoSize];
extern rotationSystem rs;

#endif