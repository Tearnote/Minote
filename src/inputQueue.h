//
// Created by hmaraszek on 12/12/2019.
//

#ifndef MINOTE_INPUTQUEUE_H
#define MINOTE_INPUTQUEUE_H

#include <mutex>
#include <queue>
#include "window.h"

// A thread-safe queue to store Window inputs
// Uses the keyHandler interface of Window
class InputQueue {
public:
	struct Input {
		int code;
		int action;
	};

	explicit InputQueue(Window&);
	~InputQueue();

private:
	Window& window;
	std::mutex mutex{};
	std::queue<Input> inputs{};

	auto push(int code, int action) -> void;
};

#endif //MINOTE_INPUTQUEUE_H
