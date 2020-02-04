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
#include "darray.h"
#include "field.h"
#include "util.h"
#include "log.h"

#define FieldWidth 10u
#define FieldHeight 22u
#define FieldHeightVisible 20u

static Model* scene = null;
static Model* minoblock = null;
static darray* tints = null;
static darray* transforms = null;
static mat4x4 identity = {0};

Field* field = null;

void playInit(void)
{
	scene = modelCreateFlat(u8"scene",
#include "meshes/scene.mesh"
	);
	minoblock = modelCreatePhong(u8"mino",
#include "meshes/mino.mesh"
	);
	tints = darrayCreate(sizeof(color4));
	transforms = darrayCreate(sizeof(mat4x4));
	mat4x4_identity(identity);

	field = fieldCreate((size2i){FieldWidth, FieldHeight});
	for (size_t i = 0; i < FieldWidth * FieldHeight; i++)
		fieldSet(field, (point2i){i % FieldWidth, i / FieldWidth},
			i < FieldWidth ? MinoGarbage : rand() % MinoSize);

	logDebug(applog, "Play state initialized");
}

void playCleanup(void)
{
	darrayDestroy(transforms);
	transforms = null;
	darrayDestroy(tints);
	tints = null;
	fieldDestroy(field);
	field = null;
	modelDestroy(minoblock);
	minoblock = null;
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
	rendererClear((color3){0.010f, 0.276f, 0.685f});
	modelDraw(scene, 1, (color4[]){Color4White}, &identity);
	for (size_t i = 0; i < FieldWidth * FieldHeight; i += 1) {
		int x = i % FieldWidth;
		int y = i / FieldWidth;
		// Flip the order of processing the left half of the playfield
		// to fix alpha sorting issues
		if (x < FieldWidth / 2)
			x = FieldWidth / 2 - x - 1;
		mino type = fieldGet(field, (point2i){x, y});
		if (type == MinoNone) continue;

		color4* tint = darrayProduce(tints);
		mat4x4* transform = darrayProduce(transforms);
		memcpy(tint->arr, minoColor(type).arr, sizeof(tint->arr));
		if (y >= FieldHeightVisible)
			tint->a /= 4.0f;
		mat4x4_identity(*transform);
		mat4x4_translate_in_place(*transform, x - (signed)(FieldWidth / 2), y,
			0.0f);
	}
	modelDraw(minoblock, darraySize(transforms), darrayData(tints),
		darrayData(transforms));
	darrayClear(tints);
	darrayClear(transforms);
}
