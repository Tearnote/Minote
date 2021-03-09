#pragma once

namespace minote::sys {

template<typename F>
	requires std::predicate<F, Window::KeyInput const&>
void Window::processInputs(F func) {
	auto const lock = std::scoped_lock{inputsMutex};

	while(!inputs.empty()) {
		if(!func(inputs.front())) return;
		inputs.pop_front();
	}
}

}
