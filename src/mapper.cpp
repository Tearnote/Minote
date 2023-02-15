#include "mapper.hpp"

#include <optional>
#include "SDL_events.h"
#include "util/vector.hpp"
#include "stx/except.hpp"
#include "util/util.hpp"
#include "sys/system.hpp"

namespace minote {

auto Mapper::convert(SDL_Event const& _e) -> std::optional<Action> {
	
	if ((_e.type != SDL_KEYDOWN) && (_e.type != SDL_KEYUP)) return std::nullopt;
	
	auto type = [&] {
		switch(_e.key.keysym.scancode) {
		
		case SDL_SCANCODE_UP:
		case SDL_SCANCODE_W:
			return Action::Type::Drop;
		
		case SDL_SCANCODE_DOWN:
		case SDL_SCANCODE_S:
			return Action::Type::Lock;
		
		case SDL_SCANCODE_LEFT:
		case SDL_SCANCODE_A:
			return Action::Type::Left;
		
		case SDL_SCANCODE_RIGHT:
		case SDL_SCANCODE_D:
			return Action::Type::Right;
		
		case SDL_SCANCODE_Z:
		case SDL_SCANCODE_J:
			return Action::Type::RotCCW;
		
		case SDL_SCANCODE_X:
		case SDL_SCANCODE_K:
			return Action::Type::RotCW;
		
		case SDL_SCANCODE_C:
		case SDL_SCANCODE_L:
			return Action::Type::RotCCW2;
		
		case SDL_SCANCODE_SPACE:
			return Action::Type::Skip;
		
		case SDL_SCANCODE_RETURN:
			return Action::Type::Accept;
		
		case SDL_SCANCODE_ESCAPE:
			return Action::Type::Back;
		
		default:
			return Action::Type::None;
		
		}
	}();
	if (type == Action::Type::None) return std::nullopt;
	
	auto state = (_e.type == SDL_KEYDOWN)?
		Action::State::Pressed :
		Action::State::Released;
	
	return Action{
		.type = type,
		.state = state,
	};
	
}

}
