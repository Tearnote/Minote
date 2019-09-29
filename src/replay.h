// Minote - replay.h
// Keeps replays, loads and saves them, helps play them back
// Also handles the replay viewer

#ifndef REPLAY_H
#define REPLAY_H

#include <stdint.h>

#include "gameplay.h"
#include "array.h"

enum replayState {
	ReplayNone,
	ReplayViewing,
	ReplayRecording,
	ReplayFinished,
	ReplayWriting,
	ReplaySize
};

struct replayHeader {
	uint8_t magic[12]; // Not zero-terminated
	uint8_t version[4]; // Not zero-terminated
	rng initialRng;
	int32_t totalFrames;
	int16_t keyframeFreq;
};

struct replayKeyframePlayer {
	int8_t state;
	int8_t x, y;
	int16_t ySub;
	int8_t type;
	int8_t preview;
	int8_t history[HISTORY_SIZE];
	int8_t rotation;
	int8_t dasDirection;
	int16_t dasCharge, dasDelay;
	int16_t lockDelay;
	int16_t clearDelay;
	int16_t spawnDelay;
	int8_t dropBonus;
	int8_t ghostEnabled;
	int8_t yGhost;
};

struct replayKeyframe {
	rng rngState;
	int8_t playfield[PLAYFIELD_H][PLAYFIELD_W];
	int8_t clearedLines[PLAYFIELD_H];
	struct replayKeyframePlayer player;
	int16_t level;
	int16_t nextLevelstop;
	int32_t score;
	int8_t combo;
	int8_t grade;
	int8_t gradeString[3];
	int8_t eligible;
	//bool cmdRaw[GameCmdSize]; // This is included separately
	bool cmdHeld[GameCmdSize];
	bool cmdPrev[GameCmdSize];
	//int8_t lastDirection; // Same
	int32_t frame;
	int64_t time;
};

struct replayInputframe {
	int8_t inputs[GameCmdSize];
	int8_t lastDirection;
};

struct replay {
	enum replayState state;
	struct replayHeader header;
	vdarray *frames; //THREADLOCAL gameplay
	bool playing; // If viewer state, are we playing or paused
	int frame;
	float speed;
};

void loadReplay(struct replay *replay);
void applyReplayInitial(struct game *game, struct replay *replay);
void applyReplayKeyframe(struct game *game, struct replay *replay, int frame);
void applyReplayInputs(struct game *game, struct replay *replay, int frame);

void pushReplayHeader(struct replay *replay, struct game *game, int keyframeFreq);
void pushReplayFrame(struct replay *replay, struct game *frame);
void saveReplay(struct replay *replay);

#endif //REPLAY_H
