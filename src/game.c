/**
 * Implementation of game.h
 * @file
 */

#include "game.h"

#include "renderer.h"
#include "window.h"
#include "mapper.h"
#include "util.h"
#include "play.h"

void* game(void* arg)
{
	(void)arg;
	mapperInit();
	rendererInit();
	playInit();

	while (windowIsOpen()) {
		mapperUpdate();
		playUpdate();

		rendererFrameBegin();
		playDraw();
		rendererFrameEnd();
	}

	playCleanup();
	rendererCleanup();
	mapperCleanup();
	return null;
}
