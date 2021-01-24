#include "mapper.hpp"

#include <stdexcept>
#include <GLFW/glfw3.h>
#include "base/util.hpp"
#include "sys/glfw.hpp"

namespace minote::engine {

using namespace base;
using namespace base::literals;

void Mapper::collectKeyInputs(sys::Window& window) {
	window.processInputs([=, this](auto const& key) {
		const auto type = [=] {
			switch (+key.keycode) {
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
		if (type == Action::Type::None) return true; // Key not recognized

		using KeyState = sys::Window::KeyInput::State;
		const auto state = [=] {
			switch (key.state) {
			case KeyState::Pressed:
				return Action::State::Pressed;
			case KeyState::Released:
				return Action::State::Released;
			default:
				throw std::logic_error{"Encountered invalid key state"};
			}
		}();

		const auto timestamp = sys::Glfw::getTime();

		try {
			actions.push_back({
				.type = type,
				.state = state,
				.timestamp = timestamp,
			});
		} catch (...) {
			throw std::runtime_error{"Mapper queue is full"};
		}
		return true;
	});
}

}
