// Minote - gameplay.c

#include "gameplay.h"

#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <time.h>

#include "state.h"
#include "input.h"
#include "mino.h"
#include "util.h"
#include "timer.h"
#include "replay.h"
#include "settings.h"
#include "logic.h"
#include "effects.h"
#include "gameplay_pure.h"

#define KEYFRAME_FREQ 60

// Thread-local copy of the game state being rendered
static struct app *snap = NULL;
// Convenience pointers
struct game *game = NULL;
struct player *player = NULL;
struct replay *replay = NULL;

int GRAVITY = 0;

static bool cmds[GameCmdSize] = {};

// Accepts inputs outside of bounds
enum mino
getPlayfieldGrid(enum mino field[PLAYFIELD_H][PLAYFIELD_W], int x, int y)
{
	if (x < 0 || x >= PLAYFIELD_W || y >= PLAYFIELD_H)
		return MinoGarbage;
	if (y < 0)
		return MinoNone;
	return field[y][x];
}

void setPlayfieldGrid(enum mino field[PLAYFIELD_H][PLAYFIELD_W],
                      int x, int y, enum mino val)
{
	if (x < 0 || x >= PLAYFIELD_W ||
	    y < 0 || y >= PLAYFIELD_H)
		return;
	field[y][x] = val;
}

// Maps generic inputs to gameplay commands
static enum gameplayCmd inputToGameCmd(enum inputType i)
{
	switch (i) {
	case InputLeft:
		return GameCmdLeft;
	case InputRight:
		return GameCmdRight;
	case InputUp:
		return GameCmdSonic;
	case InputDown:
		return GameCmdSoft;
	case InputButton1:
		return GameCmdCCW;
	case InputButton2:
		return GameCmdCW;
	case InputButton3:
		return GameCmdCCW2;
	default:
		return GameCmdNone;
	}
}

static enum replayCmd inputToReplayCmd(enum inputType i)
{
	switch (i) {
	case InputButton1:
		return ReplCmdPlay;
	case InputLeft:
		return ReplCmdBack;
	case InputRight:
		return ReplCmdFwd;
	case InputDown:
		return ReplCmdSkipBack;
	case InputUp:
		return ReplCmdSkipFwd;
	case InputButton2:
		return ReplCmdSlower;
	case InputButton3:
		return ReplCmdFaster;
	default:
		return ReplCmdNone;
	}
}

// Receive unfiltered inputs
static void processGameInput(struct input *i)
{
	enum gameplayCmd cmd = inputToGameCmd(i->type);
	switch (i->action) {
	case ActionPressed:
		// Starting and quitting is handled outside of gameplay logic
		if (i->type == InputStart
		    && getState(PhaseGameplay) == StateIntro)
			setState(PhaseGameplay, StateRunning);
		if (i->type == InputQuit) {
			setState(PhaseGameplay, StateUnstaged);
			break;
		}

		if (cmd != GameCmdNone)
			cmds[cmd] = true;
		if (cmd == GameCmdLeft || cmd == GameCmdRight)
			game->lastDirection = cmd;
		break;

	case ActionReleased:
		if (cmd != GameCmdNone)
			cmds[inputToGameCmd(i->type)] = false;
		break;
	default:
		break;
	}
}

// Fills in cmds with pending inputs
static void processInputs(void)
{
	if (replay->state == ReplayViewing) {
		applyReplayInputs(game, replay, game->frame);
	} else {
		// Receive all pending inputs
		struct input *in = NULL;
		while ((in = dequeueInput())) {
			processGameInput(in);
			free(in);
		}
	}
}

void jumpToFrame(int frame)
{
	if (frame < 0)
		frame = 0;
	if (frame >= replay->header.totalFrames)
		frame = replay->header.totalFrames - 1;

	int keyframeOffset = frame % replay->header.keyframeFreq;
	applyReplayKeyframe(game, replay, frame - keyframeOffset);
	replay->frame = game->frame;

	for (int i = 0; i < keyframeOffset; i++) {
		processInputs();
		advanceGameplayPure(game, cmds);
		replay->frame += 1;
	}
}

static void processReplayInput(struct input *i)
{
	enum replayCmd cmd = inputToReplayCmd(i->type);
	switch (i->action) {
	case ActionPressed:
		if (i->type == InputQuit) {
			setState(PhaseGameplay, StateUnstaged);
			break;
		}

		switch (cmd) {
		case ReplCmdPlay:
			replay->playing = !replay->playing;
			break;
		case ReplCmdFwd:
			if (replay->frame + 1 == replay->header.totalFrames)
				break;
			replay->frame += 1;
			processInputs();
			advanceGameplayPure(game, cmds);
			break;
		case ReplCmdBack:
			jumpToFrame(replay->frame - 1);
			break;
		case ReplCmdSkipFwd:
			jumpToFrame(
				replay->frame + (int)(60.0f * replay->speed));
			break;
		case ReplCmdSkipBack:
			jumpToFrame(
				replay->frame - (int)(60.0f * replay->speed));
			break;
		case ReplCmdSlower:
			replay->speed /= 2.0f;
			break;
		case ReplCmdFaster:
			replay->speed *= 2.0f;
			break;
		default:
			break;
		}
		logicFrequency = DEFAULT_FREQUENCY * replay->speed;
	default:
		break;
	}
}

static void processReplayInputs(void)
{
	struct input *in = NULL;
	while ((in = dequeueInput())) {
		processReplayInput(in);
		free(in);
	}
}

void initGameplay(void)
{
	snap = allocate(sizeof(*snap));
	snap->game = allocate(sizeof(*snap->game));
	game = snap->game;
	player = &game->player;
	snap->replay = allocate(sizeof(*replay));
	replay = snap->replay;
	replay->speed = 1.0f;

	initGameplayPure(game);

	if (getSettingBool(SettingReplay)) {
		replay->state = ReplayViewing;
		loadReplay(replay);
		applyReplayInitial(game, replay);
		setState(PhaseGameplay, StateRunning);
	} else {
		replay->state = ReplayRecording;
		pushReplayHeader(replay, game, KEYFRAME_FREQ);
		setState(PhaseGameplay, StateIntro);
	}
}

void cleanupGameplay(void)
{
	cleanupGameplayPure(game);
	if (replay)
		free(replay);
	if (game)
		free(game);
	if (snap)
		free(snap);
	game = NULL;
	player = NULL;
	replay = NULL;
	snap = NULL;
	setState(PhaseGameplay, StateNone);
	setState(PhaseMain, StateUnstaged);
}

void updateGameplay(void)
{
	if (replay->state == ReplayViewing)
		processReplayInputs();

	processInputs();

	if (replay->state == ReplayViewing) {
		if (replay->playing || replay->frame == -1) {
			if (replay->frame + 1 < replay->header.totalFrames) {
				advanceGameplayPure(game, cmds);
				replay->frame += 1;
			} else {
				replay->playing = false;
			}
		}
	}

	if (replay->state == ReplayRecording &&
	    getState(PhaseGameplay) == StateRunning) {
		advanceGameplayPure(game, cmds);
		pushReplayFrame(replay, game);
	}
	if (replay->state == ReplayFinished) {
		saveReplay(replay);
		replay->state = ReplayNone;
	}

	lockMutex(&appMutex);
	memcpy(app->game, game, sizeof(*app->game));
	memcpy(app->replay, replay, sizeof(*app->replay));
	unlockMutex(&appMutex);
}