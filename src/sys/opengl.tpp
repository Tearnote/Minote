/**
 * Template implementation of opengl.hpp
 * @file
 */

#pragma once

#include "sys/opengl.hpp"

#include "linmath/linmath.h"
#include "base/log.hpp"

namespace minote {

template<Standard T>
void Uniform2<T>::setLocation(Program program, char const* name)
{
	ASSERT(program);
	ASSERT(name);

	location = glGetUniformLocation(program, name);

	if (location == -1)
		L.warn(R"(Failed to get location for uniform "%s")", name);
}

template<Standard T>
void Uniform2<T>::set(const Type val)
{
	if (location == -1)
		return;

	if constexpr (std::is_same_v<Type, GLfloat>)
		glUniform1f(location, val);
	else if constexpr (std::is_same_v<Type, Vec2<GLfloat>>)
		glUniform2f(location, val.x, val.y);
	else if constexpr (std::is_same_v<Type, Vec3<GLfloat>>)
		glUniform3f(location, val.x, val.y, val.z);
	else if constexpr (std::is_same_v<Type, Vec4<GLfloat>>)
		glUniform4f(location, val.x, val.y, val.z, val.w);
	else if constexpr (std::is_same_v<Type, GLint>)
		glUniform1i(location, val);
	else if constexpr (std::is_same_v<Type, Vec2<GLint>>)
		glUniform2i(location, val.x, val.y);
	else if constexpr (std::is_same_v<Type, Vec3<GLint>>)
		glUniform3i(location, val.x, val.y, val.z);
	else if constexpr (std::is_same_v<Type, Vec4<GLint>>)
		glUniform4i(location, val.x, val.y, val.z, val.w);
	else
		L.warn("Invalid uniform type");
}

}
