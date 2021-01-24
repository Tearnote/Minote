#pragma once

namespace minote::engine {

template<typename F>
	requires std::predicate<F, Mapper::Action const&>
void Mapper::processActions(F func) {
	while(!actions.empty()) {
		if(!func(actions.front())) return;
		actions.pop_front();
	}
}

}
