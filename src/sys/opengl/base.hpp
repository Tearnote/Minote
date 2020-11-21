// Minote - sys/opengl/base.hpp
// Common types for the type-safe OpenGL wrapper

#pragma once

#include "glad/glad.h"
#include "base/math.hpp"

namespace minote {

// A type that has an equivalent in GLSL
template<typename T>
concept GLSLType =
	std::is_same_v<T, f32> ||
	std::is_same_v<T, u32> ||
	std::is_same_v<T, i32> ||
	std::is_same_v<T, vec2> ||
	std::is_same_v<T, vec3> ||
	std::is_same_v<T, vec4> ||
	std::is_same_v<T, uvec2> ||
	std::is_same_v<T, uvec3> ||
	std::is_same_v<T, uvec4> ||
	std::is_same_v<T, ivec2> ||
	std::is_same_v<T, ivec3> ||
	std::is_same_v<T, ivec4> ||
	std::is_same_v<T, mat4>;

// Common fields of all OpenGL object types
struct GLObject {

	GLuint id = 0; ///< The object has not been created if this is 0
	char const* name = nullptr; ///< Human-readable name, used in logging

	~GLObject();
};

}
