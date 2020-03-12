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
#include "aa.h"

static void gameInit(void)
{
	mapperInit();
	rendererInit();
	aaInit(AAComplex);
	playInit();
}

static void gameUpdate(void)
{
	mapperUpdate();
	playUpdate();
}

static void gameDraw(void)
{
	rendererFrameBegin();
	aaBegin();
	playDraw();
	aaEnd();
	rendererFrameEnd();
}

static void gameCleanup(void)
{
	playCleanup();
	aaCleanup();
	rendererCleanup();
	mapperCleanup();
}

void* game(void* arg)
{
	(void)arg;
	gameInit();

	while (windowIsOpen()) {
		gameUpdate();
		gameDraw();
	}

	gameCleanup();
	return null;
}
