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

/**
 * Initialize all systems and layers of the game.
 */
static void gameInit(void)
{
	mapperInit();
	rendererInit();
	playInit();
}

/**
 * Update all systems and layers in order. The inactive ones will return
 * immediately.
 */
static void gameUpdate(void)
{
	mapperUpdate();
	playUpdate();
}

/**
 * Draw all shapes on the screen. Only these will be antialiased.
 */
static void gameDrawGeometry(void)
{
	playDrawGeometry();
}

/**
 * Draw all text. This comes after antialiasing, because text comes with its
 * own AA.
 */
static void gameDrawText(void)
{
	playDrawText();
}

/**
 * Draw a full frame. This will block until vsync.
 */
static void gameDraw(void)
{
	rendererFrameBegin();
//	backgroundDraw();
	gameDrawGeometry();
//	aaResolve();
	gameDrawText();
//	postDraw();
	rendererFrameEnd();
}

/**
 * Cleanup all layers and systems, in reverse order.
 */
static void gameCleanup(void)
{
	playCleanup();
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
