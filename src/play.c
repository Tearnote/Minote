/**
 * Implementation of play.h
 * @file
 */

#include "play.h"

#include <stdlib.h>
#include "glad/glad.h"
#include <GLFW/glfw3.h>
#include "renderer.h"
#include "window.h"
#include "util.h"
#include "log.h"

static Model* scene = null;
static Model* mino = null;

static mat4x4 identity = {0};

static color4 tints[200] = {0};
static mat4x4 transforms[200] = {0};

void playInit(void)
{
	scene = modelCreateFlat(u8"scene",
#include "meshes/scene.mesh"
	);
	mino = modelCreatePhong(u8"mino",
#include "meshes/mino.mesh"
	);
	mat4x4_identity(identity);

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
	logDebug(applog, "Play state initialized");
}

void playCleanup(void)
{
	modelDestroy(mino);
	mino = null;
	modelDestroy(scene);
	scene = null;
	logDebug(applog, "Play state cleaned up");
}

void playUpdate(void)
{
	KeyInput i;
	while (windowInputDequeue(&i)) {
		logTrace(applog, u8"Input detected: %d %s",
			i.key, i.action == GLFW_PRESS ? u8"press" : u8"release");
		if (i.key == GLFW_KEY_ESCAPE) {
			logInfo(applog, u8"Esc detected, closing appwindow");
			windowClose();
		}
	}
}

void playDraw(void)
{
	rendererClear(color3ToLinear((color3){0.544f, 0.751f, 0.928f}));
	modelDraw(scene, 1, (color4[]){Color4White}, &identity);
	modelDraw(mino, 200, tints, transforms);
}
