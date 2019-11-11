// Minote - types/game.h
// Structure describing gameplay state

#ifndef TYPES_GAME_H
#define TYPES_GAME_H

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
	bool ghostEnabled;
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