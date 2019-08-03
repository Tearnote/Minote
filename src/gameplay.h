// Minote - gameplay.h
// Holds and handles gameplay logic

#ifndef GAMEPLAY_H
#define GAMEPLAY_H

#include <stdbool.h>

#include "mino.h"

#define PLAYFIELD_W 10
#define PLAYFIELD_H 20

// Frames until the first autoshift
// 2 is right after the normal shift
#define DAS_CHARGE 16

// Number of frames between autoshifts
// 1 is every frame
// 0 is instant (not supported yet)
#define DAS_DELAY 1

// Subgrid value at which the piece is dropped
#define SUBGRID 256

// Natural piece falling speed in subgrids
#define GRAVITY 16

// Piece falling speed if soft drop is held
#define SOFT_DROP 256

// Piece falling speed if sonic drop is held
#define SONIC_DROP 5120

// How many frames a piece takes to lock if it can't drop
#define LOCK_DELAY 30

// How many frames it takes for the next piece to spawn
// after the previous one is locked
#define SPAWN_DELAY 30

// Types of commands accepted by the gameplay
enum cmdType {
	CmdNone,
	CmdLeft, CmdRight,
	CmdCCW, CmdCW, CmdCCW2,
	CmdSoft, CmdSonic,
	CmdSize
};

enum playerState {
	PlayerNone,
	PlayerActive,
	PlayerClear,
	PlayerARE,
	PlayerSize
};

// Variables regarding player control
struct player {
	enum playerState state;
	int x, y;
	int ySub;
	enum pieceType type;
	enum pieceType preview;
	int rotation; // 0 to 3, 0 is spawn
	int dasDirection, dasCharge, dasDelay;
	int lockDelay;
	int spawnDelay;
};

// Complete description of the gameplay's current state
// Does not use pointers, so that it can be copied and serialized
struct game {
	enum mino playfield[PLAYFIELD_H][PLAYFIELD_W];
	struct player player;
	bool cmdPressed[CmdSize];
	bool cmdHeld[CmdSize];
};

void initGameplay(void);
void cleanupGameplay(void);

// Consume inputs and advance a single frame
void updateGameplay(void);

#endif // GAMEPLAY_H
