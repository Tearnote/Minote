// Minote - state.h

#ifndef MINOTE_STATE_H
#define MINOTE_STATE_H

#include <memory>
#include <vector>

template<typename T>
class State {
public:
	enum ResultCode {
		None,
		Delete,
		Add
	};
	struct Result {
		ResultCode code{None};
		std::unique_ptr<State<T>> next{};
	};

	bool active{true};

	explicit State(T& self, State<T>* parent = nullptr)
			:owner{self}, parent{parent} { }

	virtual ~State() = default;

	virtual auto update() -> Result = 0;

private:
	T& owner;
	State<T>* parent;
};

template<typename T>
class StateStack {
public:
	auto add(std::unique_ptr<State<T>>&&) -> void;
	auto clear() -> void;
	auto update() -> void;

private:
	std::vector<std::unique_ptr<State<T>>> states{};
};

#include "state.tcc"

#endif //MINOTE_STATE_H
