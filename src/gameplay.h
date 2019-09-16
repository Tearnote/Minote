// Minote - gameplay.h
// Holds and handles gameplay logic

#ifndef GAMEPLAY_H
#define GAMEPLAY_H

#include <stdbool.h>

#include "mino.h"
#include "timer.h"
#include "util.h"

// Define the length of a frame for the purpose of calculating the timer
// This is not equal to real time
#define TIMER_FRAMERATE 60 // in Hz
#define TIMER_FRAME (SEC / TIMER_FRAMERATE)

#define PLAYFIELD_W 10
#define PLAYFIELD_H 21
#define PLAYFIELD_H_HIDDEN 1
#define PLAYFIELD_H_VISIBLE (PLAYFIELD_H - PLAYFIELD_H_HIDDEN)

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
extern int GRAVITY;

// Piece falling speed if soft drop is held
#define SOFT_DROP 256

// Piece falling speed if sonic drop is held
#define SONIC_DROP 5120

// How many frames a piece takes to lock if it can't drop
#define LOCK_DELAY 30

// How many frames it takes since locking for the line clear check
#define CLEAR_OFFSET 4

// How many frames it takes for full lines to clear
#define CLEAR_DELAY 41

// How many frames it takes for the next piece to spawn
// after the previous one is locked
#define SPAWN_DELAY 30

// The number of times the randomizer attempts to pick a piece not in history
#define MAX_REROLLS 4

// Number of recent pieces kept by the randomizer for avoiding repeats
#define HISTORY_SIZE 4

// Types of commands accepted by the gameplay
enum gameplayCmd {
	GameCmdNone,
	GameCmdLeft, GameCmdRight,
	GameCmdCCW, GameCmdCW, GameCmdCCW2,
	GameCmdSoft, GameCmdSonic,
	GameCmdSize
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
	rng rngState;
	enum mino playfield[PLAYFIELD_H][PLAYFIELD_W];
	bool clearedLines[PLAYFIELD_H];
	struct player player;
	int level;
	int nextLevelstop;
	int score;
	int combo;
	int grade;
	char gradeString[3];
	bool eligible;
	bool cmdRaw[GameCmdSize];
	bool cmdHeld[GameCmdSize];
	bool cmdPrev[GameCmdSize];
	enum gameplayCmd lastDirection; // GameCmdLeft or GameCmdRight
	int frame;
	nsec time;
	bool started;
	bool finished;
};

void initGameplay(void);
void cleanupGameplay(void);

// Consume inputs and advance a single frame
void updateGameplay(void);

// Return the mino at the specific cell
// Accepts inputs outside of bounds
enum mino
getPlayfieldGrid(enum mino field[PLAYFIELD_H][PLAYFIELD_W], int x, int y);
void setPlayfieldGrid(enum mino field[PLAYFIELD_H][PLAYFIELD_W],
                      int x, int y, enum mino val);

// Check whether player piece can drop one grid
bool canDrop(void);

#endif // GAMEPLAY_H
