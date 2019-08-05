// Minote - gameplay.h
// Holds and handles gameplay logic

#ifndef GAMEPLAY_H
#define GAMEPLAY_H

#include <stdbool.h>

#include "mino.h"

#define PLAYFIELD_W 10
#define PLAYFIELD_H 20

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
	PlayerSpawn,
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
	int clearDelay;
	int spawnDelay;
};

// Complete description of the gameplay's current state
// Does not use pointers, so that it can be copied and serialized
struct game {
	enum mino playfield[PLAYFIELD_H][PLAYFIELD_W];
	bool clearedLines[PLAYFIELD_H];
	struct player player;
	bool cmdPressed[CmdSize];
	bool cmdHeld[CmdSize];
};

void initGameplay(void);
void cleanupGameplay(void);

// Consume inputs and advance a single frame
void updateGameplay(void);

#endif // GAMEPLAY_H
