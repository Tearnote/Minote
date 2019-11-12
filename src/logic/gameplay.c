// Minote - gameplay.c

#include "gameplay.h"

#include <stdlib.h>
#include <stdbool.h>

#include "global/state.h"
#include "main/input.h"
#include "types/mino.h"
#include "util/util.h"
#include "util/timer.h"
#include "pure.h"

// Thread-local copy of the global state
struct game *game = NULL;

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
	case InputButton4:
		return GameCmdHold;
	default:
		return GameCmdNone;
	}
}

// Receive unfiltered inputs
static void processGameInput(struct input *i)
{
	enum gameplayCmd cmd = inputToGameCmd(i->type);
	switch (i->action) {
	case ActionPressed:
		// Starting and quitting is handled outside of gameplay logic
		//if (i->type == InputStart && game->state == GameplayReady)
		//	game->state = GameplayPlaying;
		if (i->type == InputQuit) {
			setState(PhaseGame, StateUnstaged);
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
	// Receive all pending inputs
	struct input *in = NULL;
	while ((in = dequeueInput())) {
		processGameInput(in);
		free(in);
	}
}

void initGameplay(void)
{
	game = allocate(sizeof(*game));

	initGameplayPure(game);
	writeStateData(PhaseGame, game);
	setState(PhaseGame, StateRunning);
}

void cleanupGameplay(void)
{
	setState(PhaseGame, StateNone);
	if (game) {
		cleanupGameplayPure(game);
		free(game);
	}
	game = NULL;
	setState(PhaseMenu, StateStaged);
}

void updateGameplay(void)
{
	processInputs();
	advanceGameplayPure(game, cmds);
	writeStateData(PhaseGame, game);
}