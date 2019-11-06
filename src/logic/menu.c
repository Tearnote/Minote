// Minote - menu.c

#include "menu.h"

#include "global/state.h"
#include "util/util.h"
#include "main/input.h"

static struct menu *snap;

static enum menuCmd inputToMenuCmd(enum inputType i)
{
	switch (i) {
	case InputUp:
		return MenuCmdUp;
	case InputDown:
		return MenuCmdDown;
	case InputButton1:
	case InputStart:
		return MenuCmdConfirm;
	default:
		return MenuCmdNone;
	}
}

static void processMenuInput(struct input *i)
{
	if (i->action != ActionPressed)
		return;
	if (i->type == InputQuit) {
		setState(PhaseMain, StateUnstaged);
		return;
	}

	enum menuCmd cmd = inputToMenuCmd(i->type);
	switch (cmd) {
	case MenuCmdUp:
		snap->entry -= 1;
		if (snap->entry <= MenuFirst)
			snap->entry = MenuFirst + 1;
		break;
	case MenuCmdDown:
		snap->entry += 1;
		if (snap->entry >= MenuLast)
			snap->entry = MenuLast - 1;
		break;
	case MenuCmdConfirm:
		switch (snap->entry) {
		case MenuPlay:
			setState(PhaseMenu, StateUnstaged);
			setState(PhaseGameplay, StateStaged);
			break;
		case MenuQuit:
			setState(PhaseMain, StateUnstaged);
			break;
		default:
			break;
		}
		break;
	default:
		break;
	}
}

void initMenu(void)
{
	snap = allocate(sizeof(*snap));
	snap->entry = MenuFirst + 1;
	setState(PhaseMenu, StateRunning);
}

void cleanupMenu(void)
{
	setState(PhaseMenu, StateNone);
	if (snap)
		free(snap);
	snap = NULL;
}

void updateMenu(void)
{
	struct input *in = NULL;
	while ((in = dequeueInput())) {
		processMenuInput(in);
		free(in);
	}

	lockMutex(&appMutex);
	memcpy(app->menu, snap, sizeof(*app->menu));
	unlockMutex(&appMutex);
}