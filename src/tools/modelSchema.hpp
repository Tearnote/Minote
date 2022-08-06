#pragma once

#include "util/math.hpp"

namespace minote {

using TriIndexType = uint8;
using VertIndexType = uint;
using VertexType = float3;
using NormalType = uint;

constexpr auto ModelMagic = 0x10EF02FDu;
constexpr auto NormalOctBits = 16u;
constexpr auto MeshletMaxVerts = 64u;
constexpr auto MeshletMaxTris = 128u;

}
