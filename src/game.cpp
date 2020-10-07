/**
 * Implementation of game.h
 * @file
 */

#include "game.hpp"

#include "particles.hpp"
#include "renderer.hpp"
#include "sys/window.hpp"
#include "mapper.hpp"
#include "bloom.hpp"
#include "debug.hpp"
#include "world.hpp"
#include "model.hpp"
#include "base/util.hpp"
#include "play.hpp"
#include "font.hpp"
#include "text.hpp"
#include "aa.hpp"

using namespace minote;

static void gameInit(Window& window)
{
	mapperInit();
	rendererInit(window);
	fontInit();
	textInit();
	modelInit();
	bloomInit(window);
	aaInit(AAExtreme, window);
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

static void gameUpdate(Window& window)
{
	mapperUpdate(window);
#ifdef MINOTE_DEBUG
	debugUpdate();
	gameDebug();
#endif //MINOTE_DEBUG
	playUpdate(window);
	worldUpdate(window);
	particlesUpdate();
}

static void gameDraw(Window& window)
{
	rendererFrameBegin();
	aaBegin();
	playDraw();
	particlesDraw();
	aaEnd();
	glDisable(GL_DEPTH_TEST);
	textDraw();
	glEnable(GL_DEPTH_TEST);
	bloomApply();
#ifdef MINOTE_DEBUG
	debugDraw(window);
#endif //MINOTE_DEBUG
	rendererFrameEnd();
}

void game(Window& window)
{
	gameInit(window);

	while (!window.isClosing()) {
		gameUpdate(window);
		gameDraw(window);
	}

	gameCleanup();
}
