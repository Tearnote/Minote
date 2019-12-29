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

void* game(void* args)
{
	assert(args);
	GameArgs* gargs = args;
	Window* window = gargs->window;
	Log* gamelog = gargs->log;

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
		//TODO wait on vsync to limit the framerate
	}
	return null;
}
