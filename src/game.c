/**
 * Implementation of game.h
 * @file
 */

#include "game.h"

#include "renderer.h"
#include "effects.h"
#include "window.h"
#include "mapper.h"
#include "bloom.h"
#include "world.h"
#include "model.h"
#include "util.h"
#include "play.h"
#include "aa.h"

static void gameInit(void)
{
	mapperInit();
	rendererInit();
	modelInit();
	bloomInit();
	aaInit(AADist);
	worldInit();
	playInit();
	effectsInit();
}

static void gameCleanup(void)
{
	effectsCleanup();
	playCleanup();
	worldCleanup();
	aaCleanup();
	bloomCleanup();
	modelCleanup();
	rendererCleanup();
	mapperCleanup();
}

static void gameUpdate(void)
{
	mapperUpdate();
	playUpdate();
	worldUpdate();
	effectsUpdate();
}

static void gameDraw(void)
{
	rendererFrameBegin();
	aaBegin();
	playDraw();
	effectsDraw();
	aaEnd();
	bloomApply();
	rendererFrameEnd();
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
