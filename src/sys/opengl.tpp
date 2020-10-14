/**
 * Template implementation of opengl.hpp
 * @file
 */

#pragma once

#include "base/log.hpp"

namespace minote {

template<GLSLType T>
void Uniform<T>::setLocation(const Shader& shader, const char* const name)
{
	ASSERT(shader.id);
	ASSERT(name);

	location = glGetUniformLocation(shader.id, name);

	if (location == -1)
		L.warn(R"(Failed to get location for uniform "%s")", name);
}

template<GLSLType T>
void Uniform<T>::set(const Type val)
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
		glUniformMatrix4fv(location, 1, false, value_ptr(val));
	else
		ASSERT(false); // Unreachable if T concept holds
}

template<GLSLTexture T>
void Sampler<T>::setLocation(const Shader& shader, const char* const name, const TextureUnit _unit)
{
	ASSERT(shader.id);
	ASSERT(name);
	ASSERT(_unit != TextureUnit::None);

	location = glGetUniformLocation(shader.id, name);
	if (location == -1) {
		L.warn(R"(Failed to get location for sampler "%s")", name);
		return;
	}

	shader.bind();
	glUniform1i(location, +_unit - GL_TEXTURE0);
	unit = _unit;
}

template<GLSLTexture T>
void Sampler<T>::set(Type& val)
{
	val.bind(unit);
}

}
