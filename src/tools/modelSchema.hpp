#pragma once

#include "base/math.hpp"

namespace minote::tools {

using namespace base;

using IndexType = u32;
using VertexType = vec3;
using NormalType = u32; // u24 lower bits, highest u8 unused
using ColorType = u8vec4;

constexpr auto NormalOctWidth = 16u;

constexpr auto ModelElements = 8u;
constexpr auto ModelFormat = 1u;

// format: u32, must be ModelFormat
// indexCount: u32
// vertexCount: u32
// indices: IndexType[indexCount]
// vertices: VertexType[vertexCount]
// normals: NormalType[vertexCount]
// colors: ColorType[vertexCount]
// radius: f32

}
