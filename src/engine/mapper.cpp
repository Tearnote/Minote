#include "engine/mapper.hpp"

#include "base/log.hpp"
#include "sys/glfw.hpp"

namespace minote {

void Mapper::mapKeyInputs(Window& window)
{
	if (actions.isFull()) {
		L.warn("Mapper queue full");
		return;
	}

	while (const auto keyOpt = window.dequeueInput()) {
		const auto key = keyOpt.value();

		const auto type = [=] {
			switch (key.key) {
			case GLFW_KEY_UP:
			case GLFW_KEY_W:
				return Action::Type::Drop;
			case GLFW_KEY_DOWN:
			case GLFW_KEY_S:
				return Action::Type::Lock;
			case GLFW_KEY_LEFT:
			case GLFW_KEY_A:
				return Action::Type::Left;
			case GLFW_KEY_RIGHT:
			case GLFW_KEY_D:
				return Action::Type::Right;
			case GLFW_KEY_Z:
			case GLFW_KEY_J:
				return Action::Type::RotCCW;
			case GLFW_KEY_X:
			case GLFW_KEY_K:
				return Action::Type::RotCW;
			case GLFW_KEY_C:
			case GLFW_KEY_L:
				return Action::Type::RotCCW2;
			case GLFW_KEY_SPACE:
				return Action::Type::Skip;
			case GLFW_KEY_ENTER:
				return Action::Type::Accept;
			case GLFW_KEY_ESCAPE:
				return Action::Type::Back;
			default:
				return Action::Type::None;
			}
		}();
		if (type == Action::Type::None) continue; // Key not recognized

		const auto state = [=] {
			switch (key.state) {
			case GLFW_PRESS:
				return Action::State::Pressed;
			case GLFW_RELEASE:
				return Action::State::Released;
			default:
				ASSERT(false, "Encountered invalid key state");
				return Action::State::None;
			}
		}();

		const auto timestamp = Glfw::getTime();

		const Action newAction = {
			.type = type,
			.state = state,
			.timestamp = timestamp
		};
		actions.enqueue(newAction);
		if (actions.isFull()) {
			L.warn("Mapper queue full");
			return;
		}
	}
}

auto Mapper::dequeueAction() -> Action*
{
	return actions.dequeue();
}

auto Mapper::peekAction() -> Action*
{
	return actions.peek();
}

auto Mapper::peekAction() const -> const Action*
{
	return actions.peek();
}

}
