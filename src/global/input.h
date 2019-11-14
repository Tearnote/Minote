// Minote - global/input.h
// Lets threads send and receive inputs via a FIFO

#ifndef GLOBAL_INPUT_H
#define GLOBAL_INPUT_H

// Generic list of inputs used by the game
enum inputType {
	InputNone,
	InputLeft, InputRight, InputUp, InputDown,
	InputButton1, InputButton2, InputButton3, InputButton4,
	InputStart, InputQuit,
	InputSize
};

enum inputAction {
	ActionNone,
	ActionPressed,
	ActionReleased,
	ActionSize
};

struct input {
	enum inputType type;
	enum inputAction action;
	//nsec timestamp;
};

void initInput(void);
void cleanupInput(void);

void enqueueInput(struct input *i);
struct input *dequeueInput(void);

#endif //GLOBAL_INPUT_H