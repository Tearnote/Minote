/**
 * Implementation of game.h
 * @file
 */

#include "game.h"

#include <assert.h>
#include "glad/glad.h"
#include <GLFW/glfw3.h>
#include "util.h"
#include "window.h"
#include "renderer.h"

void* game(void* args)
{
	assert(args);
	GameArgs* gargs = args;
	Window* window = gargs->window;
	Log* gamelog = gargs->log;

	Renderer* renderer = rendererCreate(window, gamelog);

	while (windowIsOpen(window)) {
		KeyInput i;
		while (windowInputDequeue(window, &i)) {
			logTrace(gamelog, "Input detected: %d %s",
					i.key, i.action == GLFW_PRESS ? "press" : "release");
			if (i.key == GLFW_KEY_ESCAPE) {
				logInfo(gamelog, "Esc detected, closing window");
				windowClose(window);
			}
		}

		rendererClear(renderer, (Color3){0.262, 0.533, 0.849});
		rendererFlip(renderer);
	}

	rendererDestroy(renderer);
	renderer = null;
	return null;
}
