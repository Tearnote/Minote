/**
 * Implementation of pure.h
 * @file
 */

#include "pure.h"

#include "renderer.h"
#include "mapper.h"
#include "mino.h"
#include "util.h"
#include "log.h"

#define FieldWidth 10u ///< Width of #field
#define FieldHeight 22u ///< Height of #field
#define FieldHeightVisible 20u ///< Number of bottom rows the player can see

#define SpawnX 3 ///< X position of player piece spawn
#define SpawnY 18 ///< Y position of player piece spawn

/// A player-controlled active piece
typedef struct Player {
	mino type;
	spin rotation;
	point2i pos;
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

void pureInit(void)
{
	if (initialized) return;

	// Logic init
	tet.field = fieldCreate((size2i){FieldWidth, FieldHeight});
	tet.player.type = MinoT;
	tet.player.rotation = SpinNone;
	tet.player.pos.x = SpawnX;
	tet.player.pos.y = SpawnY;

	// Render init
	scene = modelCreateFlat(u8"scene",
#include "meshes/scene.mesh"
	);
	block = modelCreatePhong(u8"block",
#include "meshes/block.mesh"
	);
	tints = darrayCreate(sizeof(color4));
	transforms = darrayCreate(sizeof(mat4x4));

	initialized = true;
	logDebug(applog, u8"Pure sublayer initialized");
}

void pureCleanup(void)
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
	if (block) {
		modelDestroy(block);
		block = null;
	}
	if (scene) {
		modelDestroy(scene);
		scene = null;
	}
	if (tet.field) {
		fieldDestroy(tet.field);
		tet.field = null;
	}
	initialized = false;
	logDebug(applog, u8"Pure sublayer cleaned up");
}

void pureAdvance(darray* inputs)
{
	for (size_t i = 0; i < darraySize(inputs); i += 1) {
		Input* in = darrayGet(inputs, i);
		if (in->action != ActionPressed)
			continue;
		switch (in->type) {
		case InputLeft:
			tet.player.pos.x -= 1;
			break;
		case InputRight:
			tet.player.pos.x += 1;
			break;
		case InputButton1:
		case InputButton3:
			spinCounterClockwise(&tet.player.rotation);
			break;
		case InputButton2:
			spinClockwise(&tet.player.rotation);
			break;
		default:
			break;
		}
	}
}

void pureDraw(void)
{
	rendererClear((color3){0.010f, 0.276f, 0.685f});
	modelDraw(scene, 1, (color4[]){Color4White}, &IdentityMatrix);

	// Draw field contents
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
		color4Copy(*tint, minoColor(type));
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

	// Draw player piece
	piece* playerPiece = getPiece(tet.player.type, tet.player.rotation);
	for (size_t i = 0; i < MinosPerPiece; i += 1) {
		int x = (*playerPiece)[i].x + tet.player.pos.x;
		int y = (*playerPiece)[i].y + tet.player.pos.y;

		color4* tint = darrayProduce(tints);
		mat4x4* transform = darrayProduce(transforms);
		color4Copy(*tint, minoColor(tet.player.type));
		mat4x4_identity(*transform);
		mat4x4_translate_in_place(*transform, x - (signed)(FieldWidth / 2), y,
			0.0f);
	}
	modelDraw(block, darraySize(transforms), darrayData(tints),
		darrayData(transforms));
	darrayClear(tints);
	darrayClear(transforms);
}