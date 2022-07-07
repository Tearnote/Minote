#include "gfx/camera.hpp"

namespace minote::gfx {

auto Camera::direction() const -> vec3 {
	
	return vec3{
		cos(pitch) * cos(yaw),
		cos(pitch) * sin(yaw),
		sin(pitch)};
	
}

auto Camera::transform() const -> mat4 {
	
	return look(position, direction(), {0.0f, 0.0f, -1.0f});
	
}

void Camera::rotate(f32 horz, f32 vert) {
	
	yaw -= horz * lookSpeed;
	if (yaw <    0_deg) yaw += 360_deg;
	if (yaw >= 360_deg) yaw -= 360_deg;
	
	pitch += vert * lookSpeed;
	pitch = clamp(pitch, -89_deg, 89_deg);
	
}

void Camera::shift(vec3 distance) {
	
	position += distance * moveSpeed;
	
}

void Camera::roam(vec3 distance) {
	
	auto fwd = direction();
	auto right = vec3{fwd.y(), -fwd.x(), 0.0f};
	auto up = vec3{-fwd.y(), fwd.z(), fwd.x()};
	
	shift(
		distance.x() * right +
		distance.y() * up +
		distance.z() * fwd);
	
}

}
