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
	Model* triangle = modelCreate(renderer, ProgramFlat, u8"triangle", 1,
			(Triangle[]){
					(Vertex){
							.pos = {-0.5f, -0.5f, -2.0f},
							.color = {1.0f, 0.0f, 0.0f, 1.0f}
					},
					(Vertex){
							.pos = {0.5f, -0.5f, -2.0f},
							.color = {0.0f, 0.0f, 1.0f, 1.0f}
					},
					(Vertex){
							.pos = {0.0f, 0.5f, -2.0f},
							.color = {0.0f, 1.0f, 0.0f, 1.0f}
					}
			});
	mat4x4 identity;
	mat4x4_identity(identity);

	while (windowIsOpen(window)) {
		KeyInput i;
		while (windowInputDequeue(window, &i)) {
			logTrace(gamelog, u8"Input detected: %d %s",
					i.key, i.action == GLFW_PRESS ? u8"press" : u8"release");
			if (i.key == GLFW_KEY_ESCAPE) {
				logInfo(gamelog, u8"Esc detected, closing window");
				windowClose(window);
			}
		}

		rendererClear(renderer, (Color3){0.262f, 0.533f, 0.849f});
		modelDraw(renderer, triangle, 1, (Color4[]){1.0f, 1.0f, 1.0f, 1.0f},
				&identity);
		rendererFlip(renderer);
	}

	modelDestroy(renderer, triangle);
	triangle = null;
	rendererDestroy(renderer);
	renderer = null;
	return null;
}
