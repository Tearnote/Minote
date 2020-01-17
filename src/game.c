/**
 * Implementation of game.h
 * @file
 */

#include "game.h"

#include "renderer.h"
#include "window.h"
#include "util.h"
#include "play.h"

void* game(void* arg)
{
	(void)arg;
	rendererInit();
	playInit();

	while (windowIsOpen()) {
		playUpdate();

		rendererFrameBegin();
		playDraw();
		rendererFrameEnd();
	}

	playCleanup();
	rendererCleanup();
	return null;
}
