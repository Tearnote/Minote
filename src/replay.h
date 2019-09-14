// Minote - replay.h
// Keeps replays, loads and saves them, helps play them back

#ifndef REPLAY_H
#define REPLAY_H

#include "gameplay.h"

struct replayHeader {
	char *version;
	rng initialRng;
};

struct replayFramePlayer {
	enum playerState state;
	int x, y;
	enum pieceType type;
	enum pieceType preview;
	int rotation;
};

struct replayFrame {
	enum mino playfield[PLAYFIELD_H][PLAYFIELD_W];
	struct replayFramePlayer player;
	int level;
	int nextLevelstop;
	int score;
	char gradeString[3];
	bool eligible;
	bool cmdRaw[GameCmdSize];
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
void pushReplayFrame(struct game *frame);
void saveReplay(void);
void clearReplay(void);

void initReplay(void);
void cleanupReplay(void);
void updateReplay(void);

#endif //REPLAY_H
