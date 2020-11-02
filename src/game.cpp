/**
 * Implementation of game.hpp
 * @file
 */

#include "game.hpp"

#include "particles.hpp"
#include "renderer.hpp"
#include "mapper.hpp"
#include "bloom.hpp"
#include "debug.hpp"
#include "world.hpp"
#include "model.hpp"
#include "play.hpp"
#include "font.hpp"
#include "text.hpp"
#include "aa.hpp"

namespace minote {

/**
 * Temporary replacement for a settings menu.
 */
static void gameDebug(void)
{
	static AAMode aa = AANone;
	if (nk_begin(nkCtx(), "Settings", nk_rect(1070, 30, 180, 220),
		NK_WINDOW_BORDER | NK_WINDOW_MOVABLE | NK_WINDOW_MINIMIZABLE
			| NK_WINDOW_NO_SCROLLBAR)) {
		nk_layout_row_dynamic(nkCtx(), 20, 1);
		bool sync = nk_check_label(nkCtx(), "GPU synchronization",
			rendererGetSync());
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

void game(Window& window)
{
	// Initialization
	Mapper mapper;
	rendererInit(window);
	defer { rendererCleanup(); };
	fontInit();
	defer { fontCleanup(); };
	textInit();
	defer { textCleanup(); };
	modelInit();
	defer { modelCleanup(); };
	bloomInit(window);
	defer { bloomCleanup(); };
	aaInit(AANone, window);
	defer { aaCleanup(); };
	worldInit();
	defer { worldCleanup(); };
#ifdef MINOTE_DEBUG
	debugInit();
	defer { debugCleanup(); };
#endif //MINOTE_DEBUG
	playInit();
	defer { playCleanup(); };
	particlesInit();
	defer { particlesCleanup(); };

	// Main loop
	while (!window.isClosing()) {
		// Update state
		mapper.mapKeyInputs(window);
#ifdef MINOTE_DEBUG
		debugUpdate();
		gameDebug();
#endif //MINOTE_DEBUG
		playUpdate(window, mapper);
		worldUpdate(window);
		particlesUpdate();

		// Draw frame
		rendererFrameBegin();
		aaBegin();
		playDraw();
		particlesDraw();
		aaEnd();
		textQueue(FontJost, 3.0f, {6.05, 1.95, 0}, {1.0f, 1.0f, 1.0f, 0.25f}, "Text test.");
		textQueue(FontJost, 3.0f, {6, 2, 0}, {0.0f, 0.0f, 0.0f, 1.0f}, "Text test");
		glDisable(GL_DEPTH_TEST);
		textDraw();
		glEnable(GL_DEPTH_TEST);
		bloomApply();
#ifdef MINOTE_DEBUG
		debugDraw(window);
#endif //MINOTE_DEBUG
		rendererFrameEnd();
	}
}

}
