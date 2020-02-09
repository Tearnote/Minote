/**
 * Implementation of play.h
 * @file
 */

#include "play.h"

#include <stdlib.h>
#include <assert.h>
#include "renderer.h"
#include "window.h"
#include "mapper.h"
#include "darray.h"
#include "mino.h"
#include "util.h"
#include "time.h"
#include "log.h"

#define FieldWidth 10u ///< Width of #field
#define FieldHeight 22u ///< Height of #field
#define FieldHeightVisible 20u ///< Number of bottom rows the player can see

/// Frequency of game logic updates, simulated by semi-threading, in Hz
#define UpdateFrequency 59.84
/// Inverse of #UpdateFrequency, in ::nsec
#define UpdateTick (secToNsec(1) / UpdateFrequency)
/// Timestamp of the next game logic update
static nsec nextUpdate = 0;

/// A player-controlled active piece
typedef struct Player {
	mino type;
	spin rotation;
} Player;

/// A play's logical state
typedef struct Tetrion {
	Field* field;
	Player player;
} Tetrion;

static Tetrion tet = {0};

static Model* scene = null;

static Model* block = null;
static darray* tints = null; ///< of #block
static darray* transforms = null; ///< of #block

static bool initialized = false;

void playInit(void)
{
	if (initialized) return;
	scene = modelCreateFlat(u8"scene",
#include "meshes/scene.mesh"
	);
	block = modelCreatePhong(u8"block",
#include "meshes/block.mesh"
	);
	tints = darrayCreate(sizeof(color4));
	transforms = darrayCreate(sizeof(mat4x4));

	tet.field = fieldCreate((size2i){FieldWidth, FieldHeight});
	for (size_t i = 0; i < FieldWidth * FieldHeight; i++)
		fieldSet(tet.field, (point2i){i % FieldWidth, i / FieldWidth},
			i < FieldWidth ? MinoGarbage : rand() % MinoSize);

	nextUpdate = getTime() + UpdateTick;

	initialized = true;
	logDebug(applog, u8"Play state initialized");
}

void playCleanup(void)
{
	if (!initialized) return;
	if (transforms) {
		darrayDestroy(transforms);
		transforms = null;
	}
	if (tints) {
		darrayDestroy(tints);
		tints = null;
	}
	if (tet.field) {
		fieldDestroy(tet.field);
		tet.field = null;
	}
	if (block) {
		modelDestroy(block);
		block = null;
	}
	if (scene) {
		modelDestroy(scene);
		scene = null;
	}
	initialized = false;
	logDebug(applog, u8"Play state cleaned up");
}

void playUpdate(void)
{
	assert(initialized);

	// Update as many times as we need to catch up
	while (nextUpdate <= getTime()) {
		GameInput i;
		while (mapperPeek(&i)) { // Exhaust all inputs...
			if (i.timestamp <= nextUpdate)
				mapperDequeue(&i);
			else
				break; // Or abort if we encounter an input from the future

			if (i.type == InputQuit)
				windowClose();
		}
		nextUpdate += UpdateTick;
	}
}

void playDraw(void)
{
	assert(initialized);

	rendererClear((color3){0.010f, 0.276f, 0.685f});
	modelDraw(scene, 1, (color4[]){Color4White}, &IdentityMatrix);
	for (size_t i = 0; i < FieldWidth * FieldHeight; i += 1) {
		int x = i % FieldWidth;
		int y = i / FieldWidth;
		// Flip the order of processing the left half of the playfield
		// to fix alpha sorting issues
		if (x < FieldWidth / 2)
			x = FieldWidth / 2 - x - 1;
		mino type = fieldGet(tet.field, (point2i){x, y});
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
	modelDraw(block, darraySize(transforms), darrayData(tints),
		darrayData(transforms));
	darrayClear(tints);
	darrayClear(transforms);
}
