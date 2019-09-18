// Minote - replay.h
// Keeps replays, loads and saves them, helps play them back
// Also handles the replay viewer

#ifndef REPLAY_H
#define REPLAY_H

#include <stdint.h>

#include "gameplay.h"
#include "queue.h"

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
};

struct replayFramePlayer {
	int8_t state; // enum playerState
	int8_t x, y; // int
	int8_t type; // enum pieceType
	int8_t preview; // enum pieceType
	int8_t rotation; // int
};

struct replayFrame {
	int8_t playfield[PLAYFIELD_H][PLAYFIELD_W]; // enum mino
	struct replayFramePlayer player;
	int16_t level; // int
	int16_t nextLevelstop; // int
	int32_t score; // int
	int8_t gradeString[3]; // char
	int8_t eligible; // bool
	int8_t cmdRaw[GameCmdSize]; // bool
};

struct replay {
	enum replayState state;
	bool playing; // If viewer mode, are we playing or paused
	struct replayHeader header;
	queue *frames;
	int frame;
	float speed;
};

void loadReplay(void);

void pushReplayHeader(rng *initialRng);
void pushReplayFrame(struct game *frame);
void saveReplay(void);

#endif //REPLAY_H
