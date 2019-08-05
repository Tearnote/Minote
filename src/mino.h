// Minote - mino.h
// Data structures to handle minos (single blocks) and pieces (lists of offsets)

#ifndef MINO_H
#define MINO_H

#include "linmath/linmath.h"

#define PIECE_BOX 4 // Size of the bounding box all pieces fit into
#define MINOS_PER_PIECE 4
#define CENTER_COLUMN 1 // For purpose of kick exceptions

// All types of minos that can exist on the playfield
// In addition to minos the player can control,
// some extra types are reserved for later use
enum mino {
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
};

// All pieces the player can control
// Values match up with mino enum
enum pieceType {
	PieceNone,
	PieceI,
	PieceL,
	PieceO,
	PieceZ,
	PieceT,
	PieceJ,
	PieceS,
	PieceSize
};

// Coordinate struct for use with the playfield
struct coord {
	int x, y;
};

// Pieces themselves do not contain minos or colors
// That can be inferred from corresponding pieceType
typedef struct coord piece[MINOS_PER_PIECE];

// All rotation states of every piece
// [pieceType][0] is the spawn rotation
// Advances clockwise
typedef piece rotationSystem[PieceSize][4];

extern vec4 minoColors[MinoSize];
extern rotationSystem rs;

#endif // MINO_H
