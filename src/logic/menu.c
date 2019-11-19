// Minote - logic/menu.c

#include "logic/menu.h"

#include "types/menu.h"
#include "util/util.h"
#include "global/state.h"
#include "global/input.h"

static struct menu *menu;

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
		menu->entry -= 1;
		if (menu->entry <= MenuFirst)
			menu->entry = MenuFirst + 1;
		break;
	case MenuCmdDown:
		menu->entry += 1;
		if (menu->entry >= MenuLast)
			menu->entry = MenuLast - 1;
		break;
	case MenuCmdConfirm:
		switch (menu->entry) {
		case MenuPlay:
			setState(PhaseMenu, StateUnstaged);
			setState(PhaseGame, StateStaged);
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
	menu = allocate(sizeof(*menu));
	menu->entry = MenuFirst + 1;
	writeStateData(PhaseMenu, menu);
	setState(PhaseMenu, StateRunning);
}

void cleanupMenu(void)
{
	setState(PhaseMenu, StateNone);
	if (menu) {
		free(menu);
		menu = NULL;
	}
}

void updateMenu(void)
{
	struct input *in = NULL;
	while ((in = dequeueInput())) {
		processMenuInput(in);
		free(in);
	}

	writeStateData(PhaseMenu, menu);
}