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

#include <stdlib.h>

void* game(void* args)
{
	assert(args);
	GameArgs* gargs = args;
	Window* window = gargs->window;
	Log* gamelog = gargs->log;

	Renderer* renderer = rendererCreate(window, gamelog);
	ModelFlat* scene = modelCreateFlat(renderer, u8"scene",
#include "meshes/scene.mesh"
	);
	ModelPhong* mino = modelCreatePhong(renderer, u8"mino",
#include "meshes/mino.mesh"
	);
	mat4x4 identity;
	mat4x4_identity(identity);

	Color4 tints[200] = {0};
	mat4x4 transforms[200] = {0};
	for (size_t i = 0; i < 200; i += 1) {
		tints[i].r = (float)rand() / (float)RAND_MAX;
		tints[i].g = (float)rand() / (float)RAND_MAX;
		tints[i].b = (float)rand() / (float)RAND_MAX;
		tints[i].a = 1.0f;
		mat4x4_identity(transforms[i]);
		mat4x4_translate_in_place(transforms[i],
				(int)(i % 10) - 5,
				i / 10,
				0.0f);
	}

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
		modelDrawFlat(renderer, scene, 1, (Color4[]){1.0f, 1.0f, 1.0f, 1.0f},
				&identity);
		modelDrawPhong(renderer, mino, 200, tints, transforms);
		rendererFlip(renderer);
	}

	modelDestroyPhong(renderer, mino);
	mino = null;
	modelDestroyFlat(renderer, scene);
	scene = null;
	rendererDestroy(renderer);
	renderer = null;
	return null;
}
