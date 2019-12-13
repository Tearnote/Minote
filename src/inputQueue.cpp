//
// Created by hmaraszek on 12/12/2019.
//

#include "inputQueue.h"

#include <functional>
#include "window.h"
#include "log.h"

using namespace std::placeholders;

InputQueue::InputQueue(Window& w)
		:window{w}
{
	window.registerKeyHandler(std::bind(&InputQueue::push, this, _1, _2));
}

InputQueue::~InputQueue()
{
	window.unregisterKeyHandler();
}

auto InputQueue::push(int code, int action) -> void
{
	std::unique_lock<std::mutex> lock{mutex};
	//inputs.emplace(code, action);
	Log::debug("Keypress: ", code, " ", action);
}
