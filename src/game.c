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
#include "log.h"

void* game(void* arg)
{
	(void)arg;
	rendererInit();
	ModelFlat* scene = modelCreateFlat(u8"scene",
#include "meshes/scene.mesh"
	);
	ModelPhong* mino = modelCreatePhong(u8"mino",
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
			(signed)i % 10 - 5,
			i / 10,
			0.0f);
	}

	while (windowIsOpen()) {
		KeyInput i;
		while (windowInputDequeue(&i)) {
			logTrace(applog, u8"Input detected: %d %s",
				i.key, i.action == GLFW_PRESS ? u8"press" : u8"release");
			if (i.key == GLFW_KEY_ESCAPE) {
				logInfo(applog, u8"Esc detected, closing appwindow");
				windowClose();
			}
		}

		rendererFrameBegin();
		rendererClear(color3ToLinear((Color3){0.544f, 0.751f, 0.928f}));
		modelDrawFlat(scene, 1, (Color4[]){Color4White},
			&identity);
		modelDrawPhong(mino, 200, tints, transforms);
		rendererFrameEnd();
	}

	modelDestroyPhong(mino);
	mino = null;
	modelDestroyFlat(scene);
	scene = null;
	rendererCleanup();
	return null;
}
