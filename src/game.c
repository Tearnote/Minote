/**
 * Implementation of game.h
 * @file
 */

#include "game.h"

#include "particles.h"
#include "renderer.h"
#include "window.h"
#include "mapper.h"
#include "bloom.h"
#include "debug.h"
#include "world.h"
#include "model.h"
#include "util.h"
#include "play.h"
#include "font.h"
#include "text.h"
#include "aa.h"

static void gameInit(void)
{
	mapperInit();
	rendererInit();
	fontInit();
	textInit();
	modelInit();
	bloomInit();
	aaInit(AAExtreme);
	worldInit();
#ifdef MINOTE_DEBUG
	debugInit();
#endif //MINOTE_DEBUG
	playInit();
	particlesInit();
}

static void gameCleanup(void)
{
	particlesCleanup();
	playCleanup();
#ifdef MINOTE_DEBUG
	debugCleanup();
#endif //MINOTE_DEBUG
	worldCleanup();
	aaCleanup();
	bloomCleanup();
	modelCleanup();
	textCleanup();
	fontCleanup();
	rendererCleanup();
	mapperCleanup();
}

static void gameDebug(void)
{
	static AAMode aa = AAExtreme;
	if (nk_begin(nkCtx(), "Settings", nk_rect(1070, 30, 180, 220),
		NK_WINDOW_BORDER|NK_WINDOW_MOVABLE|NK_WINDOW_MINIMIZABLE|NK_WINDOW_NO_SCROLLBAR)) {
		nk_layout_row_dynamic(nkCtx(), 20, 1);
		bool sync = nk_check_label(nkCtx(), "GPU synchronization", rendererGetSync());
		if (sync != rendererGetSync())
			rendererSetSync(sync);
		nk_label(nkCtx(), "Antialiasing:", NK_TEXT_LEFT);
		if (nk_option_label(nkCtx(), "None", aa == AANone)) {
			aa = AANone;
			aaSwitch(aa);
		}
		if (nk_option_label(nkCtx(), "SMAA 1x", aa == AAFast)) {
			aa = AAFast;
			aaSwitch(aa);
		}
		if (nk_option_label(nkCtx(), "MSAA 4x", aa == AASimple)) {
			aa = AASimple;
			aaSwitch(aa);
		}
		if (nk_option_label(nkCtx(), "SMAA S2x", aa == AAComplex)) {
			aa = AAComplex;
			aaSwitch(aa);
		}
		if (nk_option_label(nkCtx(), "MSAA 8x", aa == AAExtreme)) {
			aa = AAExtreme;
			aaSwitch(aa);
		}
	}
	nk_end(nkCtx());
}

static void gameUpdate(void)
{
	mapperUpdate();
#ifdef MINOTE_DEBUG
	debugUpdate();
	gameDebug();
#endif //MINOTE_DEBUG
	playUpdate();
	worldUpdate();
	particlesUpdate();
}

static void gameDraw(void)
{
	rendererFrameBegin();
	aaBegin();
	playDraw();
	particlesDraw();
	aaEnd();
	textQueue(FontJost, 0.4f, (point3f){7.0f, 11.0f, 0.0f}, Color4Black,
		u8"Almost before we knew it, we had left the ground.");
	textQueue(FontJost, 0.6f, (point3f){8.0f, 8.8f, 0.2f}, Color4Black,
		u8"Almost before we knew it, we had left the ground.");
	textQueue(FontJost, 1.0f, (point3f){11.0f, 8.5f, 0.0f}, Color4White,
		u8"%s %s!", u8"Hello", u8"world");
	textQueue(FontJost, 3.0f, (point3f){6.5f, 6.0f, 0.0f}, Color4Black,
		u8"Serious text.");
	textQueue(FontJost, 8.0f, (point3f){6.0f, -0.5f, 0.0f}, (color4){0.3f, 0.3f, 0.3f, 1.0f},
		u8"Żółć");
	glDisable(GL_DEPTH_TEST);
	textDraw();
	glEnable(GL_DEPTH_TEST);
	bloomApply();
#ifdef MINOTE_DEBUG
	debugDraw();
#endif //MINOTE_DEBUG
	rendererFrameEnd();
}

void* game(void* arg)
{
	(void)arg;
	gameInit();

	while (windowIsOpen()) {
		gameUpdate();
		gameDraw();
	}

	gameCleanup();
	return null;
}
