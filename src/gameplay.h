// Minote - gameplay.h
// Holds and handles gameplay logic

#ifndef GAMEPLAY_H
#define GAMEPLAY_H

#include <stdbool.h>

#include "mino.h"
#include "timer.h"

#define PLAYFIELD_W 10
#define PLAYFIELD_H 21
#define PLAYFIELD_H_HIDDEN 1
#define PLAYFIELD_H_VISIBLE (PLAYFIELD_H - PLAYFIELD_H_HIDDEN)

#define HISTORY_SIZE 4

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
	PlayerSpawned, // The exact frame of piece spawn
	PlayerActive, // Piece can be freely manipulated
	PlayerClear, // Clear delay is running
	PlayerSpawn, // Spawn delay is running
	PlayerSize
};

// Variables regarding player control
struct player {
	enum playerState state;
	int x, y;
	int ySub;
	enum pieceType type;
	enum pieceType preview;
	enum pieceType history[HISTORY_SIZE];
	int rotation; // 0 to 3, 0 is spawn
	int dasDirection, dasCharge, dasDelay;
	int lockDelay;
	int clearDelay;
	int spawnDelay;
	int dropBonus;
};

// Complete description of the gameplay's current state
// Does not use pointers, so that it can be copied and serialized
struct game {
	enum mino playfield[PLAYFIELD_H][PLAYFIELD_W];
	bool clearedLines[PLAYFIELD_H];
	struct player player;
	int level;
	int nextLevelstop;
	int score;
	int combo;
	int grade;
	char gradeString[3];
	bool cmdUnfiltered[CmdSize];
	bool cmdPressed[CmdSize];
	bool cmdHeld[CmdSize];
	int frame;
	nsec time;
};

void initGameplay(void);
void cleanupGameplay(void);

// Consume inputs and advance a single frame
void updateGameplay(void);

// Return the mino at the specific cell
// Accepts inputs outside of bounds
enum mino getGrid(int x, int y);

#endif // GAMEPLAY_H
