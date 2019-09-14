// Minote - replay.h
// Keeps replays, loads and saves them, helps play them back

#ifndef REPLAY_H
#define REPLAY_H

#include <stdint.h>

#include "gameplay.h"

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
	struct replayHeader header;
	struct replayFrame *frames;
	bool playback;
	double frame;
	int totalFrames;
	float speed;
};

void initReplayQueue(void);
void cleanupReplayQueue(void);
void pushReplayHeader(rng *initialRng);
void pushReplayFrame(struct game *frame);
void saveReplay(void);
void clearReplay(void);

void initReplay(void);
void cleanupReplay(void);
void updateReplay(void);

#endif //REPLAY_H
