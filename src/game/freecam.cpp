#include "game/freecam.hpp"

#include <cmath>
#include "SDL_events.h"
#include "gfx/renderer.hpp"

namespace minote::game {

using namespace math_literals;

void Freecam::handleMouse(SDL_Event const& _e) {
	
	if (_e.type == SDL_MOUSEBUTTONDOWN && _e.button.button == SDL_BUTTON_LEFT)
		moving = true;
	if (_e.type == SDL_MOUSEBUTTONUP && _e.button.button == SDL_BUTTON_LEFT)
		moving = false;
	if (_e.type == SDL_MOUSEMOTION)
		offset += float2{float(_e.motion.xrel), float(_e.motion.yrel)}; // Accumulate lateral movement
	
}

void Freecam::handleAction(Mapper::Action _action) {
	
	auto state = (_action.state == Mapper::Action::State::Pressed);
	switch (_action.type) {
	case Mapper::Action::Type::Drop:  up       = state; return;
	case Mapper::Action::Type::Lock:  down     = state; return;
	case Mapper::Action::Type::Left:  left     = state; return;
	case Mapper::Action::Type::Right: right    = state; return;
	case Mapper::Action::Type::Skip:  floating = state; return;
	}
	
}

void Freecam::updateCamera(Camera& _camera) {
	
	// Get framerate independence multiplier
	auto framerateScale = std::min(144.0f / s_renderer->fps(), 8.0f);
	_camera.moveSpeed = 1_m / 16.0f * framerateScale;
	
	offset.y() *= -1; // Y points down in window coords but up in the world
	
	if (moving)
		_camera.rotate(offset.x(), offset.y());
	offset = float2{0.0f, 0.0f}; // Lateral movement applied, reset
	
	_camera.roam({
		float(right) - float(left),
		0.0f,
		float(up) - float(down),
	});
	_camera.shift({0.0f, 0.0f, float(floating)});
	
}

}
