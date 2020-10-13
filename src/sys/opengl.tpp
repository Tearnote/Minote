/**
 * Template implementation of opengl.hpp
 * @file
 */

#pragma once

#include "base/log.hpp"

namespace minote {

template<GLSLType T>
void Uniform2<T>::setLocation(Program program, char const* name)
{
	ASSERT(program);
	ASSERT(name);

	location = glGetUniformLocation(program, name);

	if (location == -1)
		L.warn(R"(Failed to get location for uniform "%s")", name);
}

template<GLSLType T>
void Uniform2<T>::set(const Type val)
{
	if (location == -1)
		return;

	if constexpr (std::is_same_v<Type, float>)
		glUniform1f(location, val);
	else if constexpr (std::is_same_v<Type, vec2>)
		glUniform2f(location, val.x, val.y);
	else if constexpr (std::is_same_v<Type, vec3>)
		glUniform3f(location, val.x, val.y, val.z);
	else if constexpr (std::is_same_v<Type, vec4>)
		glUniform4f(location, val.x, val.y, val.z, val.w);
	else if constexpr (std::is_same_v<Type, int>)
		glUniform1i(location, val);
	else if constexpr (std::is_same_v<Type, ivec2>)
		glUniform2i(location, val.x, val.y);
	else if constexpr (std::is_same_v<Type, ivec3>)
		glUniform3i(location, val.x, val.y, val.z);
	else if constexpr (std::is_same_v<Type, ivec4>)
		glUniform4i(location, val.x, val.y, val.z, val.w);
	else if constexpr (std::is_same_v<Type, mat4>)
		glUniform4fv(location, 1, value_ptr(val));
	else
		L.warn("Invalid uniform type");
}

}
