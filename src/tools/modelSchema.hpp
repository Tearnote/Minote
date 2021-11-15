#pragma once

#include "base/math.hpp"

namespace minote::tools {

using namespace base;

using IndexType = u32;
using VertexType = vec3;
using NormalType = vec3;
using ColorType = u16vec4;

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
