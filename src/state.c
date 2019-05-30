#include "state.h"

#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "thread.h"
#include "mino.h"
#include "util.h"

state* game;
mutex gameMutex = newMutex;
bool running;
mutex runningMutex = newMutex;

void initState(void) {
	game = allocate(1, sizeof(state));
	running = true;
	memcpy(game->playfield, (mino[PLAYFIELD_H][PLAYFIELD_W]){
		{ MinoNone, MinoNone, MinoNone, MinoNone, MinoNone, MinoNone, MinoNone, MinoNone, MinoNone, MinoNone },
		{ MinoNone, MinoNone, MinoNone, MinoNone, MinoNone, MinoNone, MinoNone, MinoNone, MinoNone, MinoNone },
		{ MinoNone, MinoNone, MinoNone, MinoNone, MinoNone, MinoNone, MinoNone, MinoNone, MinoNone, MinoNone },
		{ MinoNone, MinoNone, MinoNone, MinoNone, MinoNone, MinoNone, MinoNone, MinoNone, MinoNone, MinoNone },
		{ MinoNone, MinoNone, MinoNone, MinoNone, MinoNone, MinoNone, MinoNone, MinoNone, MinoNone, MinoNone },
		{ MinoNone, MinoNone, MinoNone, MinoNone, MinoNone, MinoNone, MinoNone, MinoNone, MinoNone, MinoNone },
		{ MinoNone, MinoNone, MinoNone, MinoNone, MinoNone, MinoNone, MinoNone, MinoNone, MinoNone, MinoNone },
		{ MinoI,    MinoNone, MinoNone, MinoNone, MinoNone, MinoNone, MinoNone, MinoNone, MinoNone, MinoNone },
		{ MinoI,    MinoNone, MinoNone, MinoNone, MinoNone, MinoNone, MinoNone, MinoNone, MinoNone, MinoNone },
		{ MinoI,    MinoNone, MinoNone, MinoNone, MinoNone, MinoNone, MinoNone, MinoNone, MinoNone, MinoNone },
		{ MinoI,    MinoNone, MinoNone, MinoNone, MinoNone, MinoNone, MinoT,    MinoNone, MinoNone, MinoNone },
		{ MinoO,    MinoO,    MinoZ,    MinoZ,    MinoNone, MinoT,    MinoL,    MinoL,    MinoNone, MinoNone },
		{ MinoO,    MinoO,    MinoT,    MinoT,    MinoNone, MinoNone, MinoNone, MinoL,    MinoNone, MinoNone },
		{ MinoI,    MinoI,    MinoO,    MinoO,    MinoNone, MinoNone, MinoT,    MinoL,    MinoNone, MinoNone },
		{ MinoI,    MinoI,    MinoO,    MinoO,    MinoS,    MinoT,    MinoT,    MinoT,    MinoNone, MinoNone },
		{ MinoI,    MinoI,    MinoO,    MinoO,    MinoS,    MinoS,    MinoNone, MinoZ,    MinoNone, MinoNone },
		{ MinoI,    MinoI,    MinoO,    MinoO,    MinoNone, MinoS,    MinoZ,    MinoZ,    MinoNone, MinoNone },
		{ MinoO,    MinoO,    MinoJ,    MinoJ,    MinoNone, MinoS,    MinoZ,    MinoJ,    MinoNone, MinoNone },
		{ MinoJ,    MinoJ,    MinoO,    MinoO,    MinoNone, MinoNone, MinoNone, MinoNone, MinoNone, MinoNone },
		{ MinoJ,    MinoJ,    MinoO,    MinoO,    MinoNone, MinoNone, MinoNone, MinoNone, MinoNone, MinoNone }
	}, sizeof(game->playfield));
	game->playerPiece.x = PLAYFIELD_W/2 - PIECE_BOX/2;
	game->playerPiece.type = PieceT;
	game->playerPiece.rotation = 0;
	game->shifting = 0;
}

void cleanupState(void) {
	free(game);
}