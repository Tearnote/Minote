#pragma once

#include "vuk/CommandBuffer.hpp"
#include "vuk/Types.hpp"
#include "vuk/Name.hpp"
#include "base/containers/string.hpp"
#include "base/concepts.hpp"
#include "base/types.hpp"
#include "base/math.hpp"

namespace minote::gfx {

using namespace base;

// Return the number a mipmaps that a square texture of the given size would have.
constexpr auto mipmapCount(u32 size) {
	return u32(floor(log2(size))) + 1;
}

// Rounded up division
template<u32 N, integral T>
constexpr auto divRoundUp(vec<N, T> _v, vec<N, T> _div) -> vec<N, T> {
	
	return (_v - T(1)) / _div + T(1);
	
}

template<u32 N, integral T>
constexpr auto divRoundUp(vec<N, T> _v, T _div) -> vec<N, T> {
	
	return (_v - vec<N, T>(1)) / _div + vec<N, T>(1);
	
}

// Create a new vuk Name by appending a provided suffix.
inline auto nameAppend(vuk::Name name, string_view suffix) -> vuk::Name {
	
	auto str = string();
	str.reserve(name.to_sv().size() + 1 + suffix.size() + 1);
	
	str.append(name.to_sv());
	str.push_back(' ');
	str.append(suffix);
	
	return vuk::Name(str);
	
}

// Conversion from vec[n] to vuk::Extent[n]D

template<arithmetic T>
constexpr auto vukExtent(vec<2, T> v) -> vuk::Extent2D { return {v[0], v[1]}; }

template<arithmetic T>
constexpr auto vukExtent(vec<3, T> v) -> vuk::Extent3D { return {v[0], v[1], v[2]}; }

// Pack two values in a u32. x is in lower bits, y in upper

constexpr auto u32Fromu16(uvec2 _v) -> u32 {
	
	return (_v.y() << 16) | _v.x();
	
}

// Shorthand for setting rendering area
inline void cmdSetViewportScissor(vuk::CommandBuffer cmd, uvec2 area) {
	
	cmd.set_viewport(0, vuk::Rect2D{ .extent = vukExtent(area) })
	   .set_scissor(0, vuk::Rect2D{ .extent = vukExtent(area) });
	
}

}
