#include "mapper.hpp"

#include "GLFW/glfw3.h"
#include "base/error.hpp"
#include "base/util.hpp"
#include "sys/glfw.hpp"

namespace minote {

using namespace base;
using namespace base::literals;

void Mapper::collectKeyInputs(sys::Window& _window) {
	
	_window.processInputs([this](auto const& key) {
		
		// Hardcoded key mapping, for now
		auto type = [=] {
			
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
		auto state = [=] {
			
			switch (key.state) {
				
			case KeyState::Pressed:
				return Action::State::Pressed;
				
			case KeyState::Released:
				return Action::State::Released;
				
			default:
				throw logic_error_fmt("Encountered invalid key state: {}", +key.state);
				
			}
			
		}();
		
		// No issues, add the translated event
		m_actions.push({
			.type = type,
			.state = state,
			.timestamp = sys::Glfw::getTime()});
		return true;
		
	});
	
}

}
