#include "gfx/camera.hpp"

namespace minote {

using namespace math_literals;

auto Camera::direction() const -> float3 {
	
	return float3{
		cos(pitch) * cos(yaw),
		cos(pitch) * sin(yaw),
		sin(pitch)};
	
}

auto Camera::view() const -> float4x4 {
	
	return look(position, direction(), {0.0f, 0.0f, -1.0f});
	
}

auto Camera::projection() const -> float4x4 {
	
	return perspective(verticalFov, float(viewport.x()) / float(viewport.y()), nearPlane);
	
}

void Camera::rotate(float horz, float vert) {
	
	yaw -= horz * lookSpeed;
	if (yaw <    0_deg) yaw += 360_deg;
	if (yaw >= 360_deg) yaw -= 360_deg;
	
	pitch += vert * lookSpeed;
	pitch = clamp(pitch, -89_deg, 89_deg);
	
}

void Camera::shift(float3 distance) {
	
	position += distance * moveSpeed;
	
}

void Camera::roam(float3 distance) {
	
	auto fwd = direction();
	auto right = float3{fwd.y(), -fwd.x(), 0.0f};
	auto up = float3{-fwd.y(), fwd.z(), fwd.x()};
	
	shift(
		distance.x() * right +
		distance.y() * up +
		distance.z() * fwd);
	
}

}
