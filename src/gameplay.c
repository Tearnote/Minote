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
#include "logic.h"
#include "effects.h"
#include "gameplay_pure.h"

// Thread-local copy of the game state being rendered
static struct app *snap = NULL;
// Convenience pointers
struct game *game = NULL;
struct player *player = NULL;

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
	// Receive all pending inputs
	struct input *in = NULL;
	while ((in = dequeueInput())) {
		processGameInput(in);
		free(in);
	}
}

void initGameplay(void)
{
	snap = allocate(sizeof(*snap));
	snap->game = allocate(sizeof(*snap->game));
	game = snap->game;
	player = &game->player;

	initGameplayPure(game);
	setState(PhaseGameplay, StateIntro);
}

void cleanupGameplay(void)
{
	cleanupGameplayPure(game);
	if (game)
		free(game);
	if (snap)
		free(snap);
	game = NULL;
	player = NULL;
	snap = NULL;
	setState(PhaseGameplay, StateNone);
	setState(PhaseMain, StateUnstaged);
}

void updateGameplay(void)
{
	processInputs();
	if (getState(PhaseGameplay) == StateRunning)
		advanceGameplayPure(game, cmds);

	lockMutex(&appMutex);
	memcpy(app->game, game, sizeof(*app->game));
	unlockMutex(&appMutex);
}