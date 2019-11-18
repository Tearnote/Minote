// Minote - logic/gameplay.c

#include "logic/gameplay.h"

#include <stdbool.h>

#include "util/timer.h"
#include "global/state.h"
#include "global/input.h"
#include "logic/pure.h"

// Thread-local copy of the global state
struct game *game = NULL;

// State of the inputs
static bool cmds[GameCmdSize] = {};

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

static void processGameInput(struct input *i)
{
	enum gameplayCmd cmd = inputToGameCmd(i->type);
	switch (i->action) {
	case ActionPressed:
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
			cmds[cmd] = false;
		break;
	default:
		break;
	}
}

// Fills in cmds with inputs from the queue
static void processInputs(void)
{
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
		game = NULL;
	}
	setState(PhaseMenu, StateStaged);
}

void updateGameplay(void)
{
	processInputs();
	advanceGameplayPure(game, cmds);
	writeStateData(PhaseGame, game);
}