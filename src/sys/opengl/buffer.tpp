#pragma once

#include "base/log.hpp"
#include "sys/opengl/state.hpp"

namespace minote {

template<copy_constructible T, GLenum _target>
void
BufferBase<T, _target>::create(char const* const _name, bool const _dynamic)
{
	DASSERT(!id);
	DASSERT(_name);

	glGenBuffers(1, &id);
#ifndef NDEBUG
	glObjectLabel(GL_BUFFER, id, std::strlen(_name), _name);
#endif //NDEBUG

	name = _name;
	dynamic = _dynamic;

	L.debug(R"({} vertex buffer "{}" created)",
		dynamic ? "Dynamic" : "Static", name);
}

template<copy_constructible T, GLenum _target>
void BufferBase<T, _target>::destroy()
{
	DASSERT(id);

	detail::state.deleteBuffer(Target, id);
	id = 0;
	dynamic = false;
	uploaded = false;

	L.debug(R"(Vertex buffer "{}" destroyed)", name);
	name = nullptr;
}

template<copy_constructible T, GLenum _target>
void BufferBase<T, _target>::upload(span<Type const> const data)
{
	DASSERT(id);
	DASSERT(dynamic == true || uploaded == false);
	if (data.empty())
		return;

	bind();
	GLenum const usage = dynamic ? GL_STREAM_DRAW : GL_STATIC_DRAW;
	if (dynamic && uploaded) {
		glBufferData(Target, data.size_bytes(), nullptr, usage);
		glBufferSubData(Target, 0, data.size_bytes(), data.data());
	} else {
		glBufferData(Target, data.size_bytes(), data.data(), usage);
		uploaded = true;
	}
}

template<copy_constructible T, GLenum _target>
void BufferBase<T, _target>::bind() const
{
	DASSERT(id);

	detail::state.bindBuffer(Target, id);
}

}
