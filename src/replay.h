// Minote - replay.h
// Keeps replays, loads and saves them, helps play them back

#ifndef REPLAY_H
#define REPLAY_H

#include "gameplay.h"

enum replayCmd {
	ReplCmdNone,
	ReplCmdPlay,
	ReplCmdFwd, ReplCmdBack,
	ReplCmdSkipFwd, ReplCmdSkipBack,
	ReplCmdFaster, ReplCmdSlower,
	ReplCmdSize
};

struct replay {
	bool playback;
	double frame;
	int totalFrames;
	float speed;
};

void initReplayQueue(void);
void cleanupReplayQueue(void);
void pushReplayState(struct game *state);
void saveReplay(void);
void clearReplay(void);

void initReplay(void);
void cleanupReplay(void);
void updateReplay(void);

#endif //REPLAY_H
