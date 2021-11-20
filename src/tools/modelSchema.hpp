#pragma once

#include "base/math.hpp"

namespace minote::tools {

using namespace base;

using IndexType = u32;
using VertexType = vec3;
using NormalType = u32;

constexpr auto ModelFormat = 2u;
constexpr auto NormalOctBits = 16u;

// An object file is an ordered map:
//  - format: u32, must be equal to ModelFormat
//  - materials: array of ordered maps:
//    - color: f32[4]
//    - metalness: f32
//    - roughness: f32
//  - meshes: array of ordered maps:
//    - materialIdx: u32
//    - radius: f32
//    - indexCount: u32
//    - vertexCount: u32
//    - indices: binary blob: IndexType[indexCount]
//    - vertices: binary blob: VertexType[vertexCount]
//    - normals: binary blob: NormalType[vertexCount]

}
