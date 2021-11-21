#pragma once

#include "base/types.hpp"
#include "base/math.hpp"
#include "tools/modelSchema.hpp"

namespace minote::tools {

using namespace base;

auto octEncode(vec3 _norm) -> u32 {
	
	_norm /= abs(_norm.x()) + abs(_norm.y()) + abs(_norm.z());
	auto wrapped = _norm.z() >= 0.0f?
		vec2{_norm.x(), _norm.y()} :
		vec2{
			(1.0f - abs(_norm.y())) * (_norm.x() >= 0.0f? 1.0f : -1.0f),
			(1.0f - abs(_norm.x())) * (_norm.y() >= 0.0f? 1.0f : -1.0f) };
	auto v = vec2(0.5f) + wrapped * 0.5f;
	
	auto mu = (1u << NormalOctBits) - 1u;
	auto d = v * f32(mu) + vec2(0.5f);
	return u32(floor(d.x())) | (u32(floor(d.y())) << NormalOctBits);
	
}

}