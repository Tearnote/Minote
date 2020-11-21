#pragma once

#include "base/log.hpp"
#include "sys/opengl/state.hpp"

namespace minote {

template<TriviallyCopyable T, GLenum _target>
void
BufferBase<T, _target>::create(char const* const _name, bool const _dynamic)
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
		dynamic ? "Dynamic" : "Static", name);
}

template<TriviallyCopyable T, GLenum _target>
void BufferBase<T, _target>::destroy()
{
	ASSERT(id);

	detail::state.deleteBuffer(Target, id);
	id = 0;
	dynamic = false;
	uploaded = false;

	L.debug(R"(Vertex buffer "%s" destroyed)", name);
	name = nullptr;
}

template<TriviallyCopyable T, GLenum _target>
template<template<TriviallyCopyable, size_t> typename Arr, size_t N>
	requires ArrayContainer<Arr, T, N>
void BufferBase<T, _target>::upload(Arr<Type, N> const& data)
{
	ASSERT(id);
	ASSERT(dynamic == true || uploaded == false);
	if (!data.size()) return;

	bind();
	GLenum const usage = dynamic ? GL_STREAM_DRAW : GL_STATIC_DRAW;
	GLsizeiptr const size = sizeof(Type) * data.size();
	if (dynamic && uploaded) {
		glBufferData(Target, size, nullptr, usage);
		glBufferSubData(Target, 0, size, data.data());
	} else {
		glBufferData(Target, size, data.data(), usage);
		uploaded = true;
	}
}

template<TriviallyCopyable T, GLenum _target>
void BufferBase<T, _target>::upload(size_t elements, Type data[])
{
	ASSERT(data);
	ASSERT(id);
	ASSERT(dynamic == true || uploaded == false);
	if (!elements)
		return;

	bind();
	GLenum const usage = dynamic ? GL_STREAM_DRAW : GL_STATIC_DRAW;
	GLsizeiptr const size = sizeof(Type) * elements;
	if (dynamic && uploaded) {
		glBufferData(Target, size, nullptr, usage);
		glBufferSubData(Target, 0, size, data);
	} else {
		glBufferData(Target, size, data, usage);
		uploaded = true;
	}
}

template<TriviallyCopyable T, GLenum _target>
void BufferBase<T, _target>::bind() const
{
	ASSERT(id);

	detail::state.bindBuffer(Target, id);
}

}
