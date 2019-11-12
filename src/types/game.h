// Minote - types/game.h
// Structure describing gameplay state

#ifndef TYPES_GAME_H
#define TYPES_GAME_H

#include <stdbool.h>

#include "types/mino.h"
#include "util/util.h"
#include "util/timer.h"

// Number of recent pieces kept by the randomizer for avoiding repeats
#define HISTORY_SIZE 4

enum playerState {
	PlayerNone,
	PlayerSpawned, // The exact frame of piece spawn
	PlayerActive, // Piece can be freely manipulated
	PlayerClear, // Clear delay is running
	PlayerSpawn, // Spawn delay is running
	PlayerSize
};

struct laws {
	bool ghost; // Whether the ghost is visible
	int gravity; // Neutral drop speed in subgrids per frame
	int softDrop; // Soft drop speed in subgrids per frame
	int sonicDrop; // Sonic drop speed in subgrids per frame
	int dasCharge; // Frame# of the first autoshift, incl. start frame
	int dasDelay; // Frames between autoshifts //TODO 0 for instant
	int lockDelay; // Frames it takes for a resting piece to lock
	int clearOffset; // Frames between lock and clear check
	int clearDelay; // Frames between clear and thump
	int spawnDelay; // Frames between lock and spawn (excluding clear delay)
};

struct player {
	enum playerState state;
	struct laws laws;
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
	int yGhost;
};

enum gameplayCmd {
	GameCmdNone,
	GameCmdLeft, GameCmdRight,
	GameCmdCCW, GameCmdCW, GameCmdCCW2,
	GameCmdSoft, GameCmdSonic,
	GameCmdHold,
	GameCmdSize
};

enum gameplayState {
	GameplayNone,
	GameplayReady,
	GameplayPlaying,
	GameplayOutro,
	GameplaySize
};

struct game {
	enum gameplayState state;
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
	int ready;
};

#endif //TYPES_GAME_H