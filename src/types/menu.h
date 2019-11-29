// Minote - types/menu.h
// Structure describing menu state

#ifndef TYPES_MENU_H
#define TYPES_MENU_H

enum menuEntry {
	MenuNone,
	MenuFirst, // for bounds-checking
	MenuPlay,
	MenuQuit,
	MenuLast, // for bounds-checking
	MenuSize
};

enum menuCmd {
	MenuCmdNone,
	MenuCmdUp, MenuCmdDown,
	MenuCmdConfirm,
	MenuCmdSize
};

struct menu {
	enum menuEntry entry;
};

#endif //TYPES_MENU_H
