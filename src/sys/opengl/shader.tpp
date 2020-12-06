#pragma once

#include "base/log.hpp"

namespace minote {

template<GLSLType T>
void Uniform<T>::setLocation(Shader const& shader, char const* const name)
{
	DASSERT(shader.id);
	DASSERT(name);

	location = glGetUniformLocation(shader.id, name);
	shaderId = shader.id;

	if (location == -1)
		L.warn(R"(Failed to get location for uniform "{}")", name);
}

template<GLSLType T>
void Uniform<T>::set(Type const _value)
{
	if (location == -1 || _value == value || shaderId == 0) return;

	detail::state.bindShader(shaderId);
	if constexpr (std::is_same_v<Type, float>)
		glUniform1f(location, _value);
	else if constexpr (std::is_same_v<Type, vec2>)
		glUniform2f(location, _value.x, _value.y);
	else if constexpr (std::is_same_v<Type, vec3>)
		glUniform3f(location, _value.x, _value.y, _value.z);
	else if constexpr (std::is_same_v<Type, vec4>)
		glUniform4f(location, _value.x, _value.y, _value.z, _value.w);
	else if constexpr (std::is_same_v<Type, int>)
		glUniform1i(location, _value);
	else if constexpr (std::is_same_v<Type, ivec2>)
		glUniform2i(location, _value.x, _value.y);
	else if constexpr (std::is_same_v<Type, ivec3>)
		glUniform3i(location, _value.x, _value.y, _value.z);
	else if constexpr (std::is_same_v<Type, ivec4>)
		glUniform4i(location, _value.x, _value.y, _value.z, _value.w);
	else if constexpr (std::is_same_v<Type, mat4>)
		glUniformMatrix4fv(location, 1, false, value_ptr(_value));
	else
		L.fail("Unknown uniform type");
	value = _value;
}

template<template<PixelFmt> typename T>
void Sampler<T>::setLocation(Shader const& shader, char const* const name, TextureUnit const _unit)
{
	DASSERT(shader.id);
	DASSERT(name);
	DASSERT(_unit != TextureUnit::None);

	location = glGetUniformLocation(shader.id, name);
	if (location == -1) {
		L.warn(R"(Failed to get location for sampler "{}")", name);
		return;
	}

	shader.bind();
	glUniform1i(location, +_unit - GL_TEXTURE0);
	unit = _unit;
}

template<template<PixelFmt> typename T>
template<PixelFmt F>
void Sampler<T>::set(T<F>& val)
{
	val.bind(unit);
}

template<BufferTextureType T>
void BufferSampler::set(BufferTexture<T>& val)
{
	val.bind(unit);
}

}
