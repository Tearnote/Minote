// Minote - state.tcc

#ifndef MINOTE_STATE_TCC
#define MINOTE_STATE_TCC

#include "state.h"

template<typename T>
auto StateStack<T>::add(std::unique_ptr<State<T>>&& s) -> void
{
	states.emplace_back(std::move(s));
}

template<typename T>
auto StateStack<T>::clear() -> void
{
	states.clear();
}

template<typename T>
auto StateStack<T>::update() -> void
{
	for (auto& s: states)
		s->update();
}

#endif //MINOTE_STATE_TCC
