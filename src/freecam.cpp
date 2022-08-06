#include "freecam.hpp"

#include "gfx/renderer.hpp"

namespace minote {

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

void Freecam::updateCamera(Camera& _camera) {
	
	// Get framerate independence multiplier
	auto framerateScale = min(144.0f / s_renderer->fps(), 8.0f);
	_camera.moveSpeed = 1_m / 16.0f * framerateScale;
	
	offset.y() = -offset.y();
	
	if (moving)
		_camera.rotate(offset.x(), offset.y());
	offset = float2(0.0f); // Lateral movement applied, reset
	
	_camera.roam({
		float(right) - float(left),
		0.0f,
		float(up) - float(down)});
	_camera.shift({0.0f, 0.0f, float(floating)});
	
}

}
