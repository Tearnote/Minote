#pragma once

#include "util/math.hpp"

namespace minote {

using TriIndexType = u8;
using VertIndexType = u32;
using VertexType = vec3;
using NormalType = u32;

constexpr auto ModelMagic = 0x10EF02FDu;
constexpr auto NormalOctBits = 16u;
constexpr auto MeshletMaxVerts = 64u;
constexpr auto MeshletMaxTris = 128u;

}
