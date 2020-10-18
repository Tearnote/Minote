/**
 * Template implementation of opengl.hpp
 * @file
 */

#pragma once

#include "base/log.hpp"

namespace minote {

template<Trivial T>
void VertexBuffer<T>::create(const char* const _name, const bool _dynamic)
{
		ASSERT(!id);
		ASSERT(_name);

	glGenBuffers(1, &id);
#ifndef NDEBUG
	glObjectLabel(GL_BUFFER, id, std::strlen(_name), _name);
#endif //NDEBUG

	name = _name;
	dynamic = _dynamic;

	L.debug(R"(%s vertex buffer "%s" created)",
		dynamic? "Dynamic" : "Static", name);
}

template<Trivial T>
void VertexBuffer<T>::destroy()
{
#ifndef NDEBUG
	if (!id) {
		L.warn("Tried to destroy a vertex buffer that has not been created");
		return;
	}
#endif //NDEBUG

	glDeleteBuffers(1, &id);
	id = 0;
	dynamic = false;
	uploaded = false;

	L.debug(R"(Vertex buffer "%s" destroyed)", name);
	name = nullptr;
}

template<Trivial T>
template<std::size_t N>
void VertexBuffer<T>::upload(varray<Type, N> data)
{
	ASSERT(id);
	ASSERT(dynamic == true || uploaded == false);
	if(!data.size)
		return;

	bind();
	const GLenum usage = dynamic? GL_STREAM_DRAW : GL_STATIC_DRAW;
	const GLsizeiptr size = sizeof(Type) * data.size;
	if (dynamic && uploaded) {
		glBufferData(GL_ARRAY_BUFFER, size, nullptr, usage);
		glBufferSubData(GL_ARRAY_BUFFER, 0, size, data.data());
	} else {
		glBufferData(GL_ARRAY_BUFFER, size, data.data(), usage);
		uploaded = true;
	}
}

template<Trivial T>
template<std::size_t N>
void VertexBuffer<T>::upload(std::array<Type, N> data)
{
	ASSERT(id);
	ASSERT(dynamic == false || uploaded == false);
	if (!data.size())
		return;

	bind();
	const GLenum usage = dynamic? GL_STREAM_DRAW : GL_STATIC_DRAW;
	const GLsizeiptr size = sizeof(Type) * N;
	if (dynamic && uploaded) {
		glBufferData(GL_ARRAY_BUFFER, size, nullptr, usage);
		glBufferSubData(GL_ARRAY_BUFFER, 0, size, data.data());
	} else {
		glBufferData(GL_ARRAY_BUFFER, size, data.data(), usage);
		uploaded = true;
	}
}

template<Trivial T>
void VertexBuffer<T>::upload(std::size_t elements, Type* data)
{
	ASSERT(data);
	ASSERT(id);
	ASSERT(dynamic == true || uploaded == false);
	if (!elements)
		return;

	bind();
	const GLenum usage = dynamic? GL_STREAM_DRAW : GL_STATIC_DRAW;
	const GLsizeiptr size = sizeof(Type) * elements;
	if (dynamic && uploaded) {
		glBufferData(GL_ARRAY_BUFFER, size, nullptr, usage);
		glBufferSubData(GL_ARRAY_BUFFER, 0, size, data);
	} else {
		glBufferData(GL_ARRAY_BUFFER, size, data, usage);
		uploaded = true;
	}
}

template<Trivial T>
void VertexBuffer<T>::bind() const
{
	ASSERT(id);

	glBindBuffer(GL_ARRAY_BUFFER, id);
}

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
