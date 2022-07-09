#include "freecam.hpp"

#include "gfx/engine.hpp"

namespace minote {

void Freecam::handleMouse(SDL_Event const& _e) {
	
	if (_e.type == SDL_MOUSEBUTTONDOWN && _e.button.button == SDL_BUTTON_LEFT)
		moving = true;
	if (_e.type == SDL_MOUSEBUTTONUP && _e.button.button == SDL_BUTTON_LEFT)
		moving = false;
	if (_e.type == SDL_MOUSEMOTION)
		offset += vec2{f32(_e.motion.xrel), f32(_e.motion.yrel)}; // Accumulate lateral movement
	
}

void Freecam::handleAction(Mapper::Action _action) {
	
	auto state = (_action.state == Mapper::Action::State::Pressed);
	if (_action.type == Mapper::Action::Type::Drop)
		up = state;
	if (_action.type == Mapper::Action::Type::Lock)
		down = state;
	if (_action.type == Mapper::Action::Type::Left)
		left = state;
	if (_action.type == Mapper::Action::Type::Right)
		right = state;
	if (_action.type == Mapper::Action::Type::Skip)
		floating = state;
	
}

void Freecam::updateCamera() {
	
	// Get framerate independence multiplier
	auto framerateScale = min(144.0f / s_engine->fps(), 8.0f);
	s_engine->camera().moveSpeed = 1_m / 16.0f * framerateScale;
	
	offset.y() = -offset.y();
	
	if (moving)
		s_engine->camera().rotate(offset.x(), offset.y());
	offset = vec2(0.0f); // Lateral movement applied, reset
	
	s_engine->camera().roam({
		float(right) - float(left),
		0.0f,
		float(up) - float(down)});
	s_engine->camera().shift({0.0f, 0.0f, float(floating)});
	
}

}
