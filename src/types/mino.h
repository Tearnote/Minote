// Minote - types/mino.h
// Data structures to handle minos (single blocks) and pieces (lists of offsets)

#ifndef TYPES_MINO_H
#define TYPES_MINO_H

#include "linmath/linmath.h"

#define PIECE_BOX 4 // Size of the bounding box all pieces fit into
#define MINOS_PER_PIECE 4
#define CENTER_COLUMN 1 // For purpose of kick exceptions

#define PLAYFIELD_W 10
#define PLAYFIELD_H 21
#define PLAYFIELD_H_HIDDEN 1
#define PLAYFIELD_H_VISIBLE (PLAYFIELD_H - PLAYFIELD_H_HIDDEN)

#define SUBGRID 256 // Number of "subpixels" in a playfield grid

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

// Return the mino at the specific cell
// Accepts inputs outside of bounds
enum mino
getPlayfieldGrid(enum mino field[PLAYFIELD_H][PLAYFIELD_W], int x, int y);
void setPlayfieldGrid(enum mino field[PLAYFIELD_H][PLAYFIELD_W],
                      int x, int y, enum mino val);

#endif //TYPES_MINO_H
