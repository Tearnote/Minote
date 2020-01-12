/**
 * Implementation of game.h
 * @file
 */

#include "game.h"

#include <assert.h>
#include "glad/glad.h"
#include <GLFW/glfw3.h>
#include "visualtypes.h"
#include "renderer.h"
#include "window.h"
#include "util.h"
#include "play.h"
#include "log.h"

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
