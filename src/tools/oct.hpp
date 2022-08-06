#pragma once

#include "util/types.hpp"
#include "util/math.hpp"
#include "tools/modelSchema.hpp"

namespace minote {

auto octEncode(float3 _norm) -> uint {
	
	_norm /= abs(_norm.x()) + abs(_norm.y()) + abs(_norm.z());
	auto wrapped = _norm.z() >= 0.0f?
		float2{_norm.x(), _norm.y()} :
		float2{
			(1.0f - abs(_norm.y())) * (_norm.x() >= 0.0f? 1.0f : -1.0f),
			(1.0f - abs(_norm.x())) * (_norm.y() >= 0.0f? 1.0f : -1.0f) };
	auto v = float2{0.5f, 0.5f} + wrapped * 0.5f;
	
	auto mu = (1u << NormalOctBits) - 1u;
	auto d = v * float(mu) + float2{0.5f, 0.5f};
	return uint(floor(d.x())) | (uint(floor(d.y())) << NormalOctBits);
	
}

}
