// Minote - menu.h
// Handles main menu logic

#ifndef MENU_H
#define MENU_H

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

void initMenu(void);
void cleanupMenu(void);

void updateMenu(void);

#endif // MENU_H