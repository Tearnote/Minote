/**
 * Template implementation of opengl.hpp
 * @file
 */

#pragma once

#include "base/log.hpp"

namespace minote {

namespace detail {

inline auto attachmentIndex(Attachment const attachment) -> std::size_t
{
	switch(attachment) {
	case Attachment::DepthStencil:
		return 16;
#ifndef NDEBUG
	case Attachment::None:
		L.warn("Invalid attachment index %d", +attachment);
		return -1;
#endif //NDEBUG
	default:
		return (+attachment) - (+Attachment::Color0);
	}
}

/**
 * Helper function to retrieve a texture pointer at a specified attachment point
 * @param f Framebuffer object
 * @param attachment Attachment point
 * @return The pointer to texture at given attachment point (can be nullptr)
 */
inline auto getAttachment(Framebuffer& f, Attachment const attachment) -> TextureBase const*&
{
	return f.attachments[attachmentIndex(attachment)];
}

inline auto getAttachment(Framebuffer const& f, Attachment const attachment) -> TextureBase const*
{
	return f.attachments[attachmentIndex(attachment)];
}

}

template<Trivial T>
void VertexBuffer<T>::create(char const* const _name, bool const _dynamic)
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
	GLenum const usage = dynamic? GL_STREAM_DRAW : GL_STATIC_DRAW;
	GLsizeiptr const size = sizeof(Type) * data.size;
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
	GLenum const usage = dynamic? GL_STREAM_DRAW : GL_STATIC_DRAW;
	GLsizeiptr const size = sizeof(Type) * N;
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
	GLenum const usage = dynamic? GL_STREAM_DRAW : GL_STATIC_DRAW;
	GLsizeiptr const size = sizeof(Type) * elements;
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

template<PixelFmt F>
void Texture<F>::create(char const* const _name, ivec2 const _size)
{
	ASSERT(!id);
	ASSERT(_name);
	ASSERT(Format != PixelFmt::None);

	glGenTextures(1, &id);
#ifndef NDEBUG
	glObjectLabel(GL_TEXTURE, id, std::strlen(_name), _name);
#endif //NDEBUG
	name = _name;
	glBindTexture(GL_TEXTURE_2D, id);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	setFilter(Filter::Linear);
	resize(_size);

	L.debug(R"(Texture "%s" created)", name);
}

template<PixelFmt F>
void Texture<F>::destroy()
{
#ifndef NDEBUG
	if (!id) {
		L.warn("Tried to destroy a texture that has not been created");
		return;
	}
#endif //NDEBUG

	glDeleteTextures(1, &id);
	id = 0;
	size = {0, 0};
	filter = Filter::None;

	L.debug(R"(Texture "%s" destroyed)", name);
	name = nullptr;
}

template<PixelFmt F>
void Texture<F>::setFilter(Filter const _filter)
{
	ASSERT(_filter != Filter::None);
	if (filter == _filter)
		return;

	glBindTexture(GL_TEXTURE_2D, id);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, +_filter);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, +_filter);
	filter = _filter;
}

template<PixelFmt F>
void Texture<F>::resize(ivec2 const _size)
{
	ASSERT(_size.x > 0 && _size.y > 0);
	ASSERT(id);
	if (size == _size)
		return;

	glBindTexture(GL_TEXTURE_2D, id);
	glTexImage2D(GL_TEXTURE_2D, 0, +Format,
		_size.x, _size.y, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
	size = _size;
}

template<PixelFmt F>
template<UploadFmt T, std::size_t N>
void Texture<F>::upload(std::array<T, N> const& data)
{
	ASSERT(data.size() == size.x * size.y);
	ASSERT(id);
	ASSERT(size.x > 0 && size.y > 0);
	ASSERT(Format != PixelFmt::DepthStencil);

	constexpr GLenum channels = [] {
		if constexpr (std::is_same_v<T, std::uint8_t>)
			return GL_RED;
		else if constexpr (std::is_same_v<T, u8vec2>)
			return GL_RG;
		else if constexpr (std::is_same_v<T, u8vec3>)
			return GL_RGB;
		else if constexpr (std::is_same_v<T, u8vec4>)
			return GL_RGBA;
		else
			ASSERT("Invalid texture upload type");
		return GL_NONE;
	}();

	glBindTexture(GL_TEXTURE_2D, id);
	glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, size.x, size.y,
		channels, GL_UNSIGNED_BYTE, data.data());
}

template<PixelFmt F>
template<UploadFmt T>
void Texture<F>::upload(T const data[], int const _channels)
{
	ASSERT(data);
	ASSERT(_channels >= 0 && _channels <= 4);
	ASSERT(id);
	ASSERT(size.x > 0 && size.y > 0);
	ASSERT(Format != PixelFmt::DepthStencil);

	const GLenum channels = [=] {
		if (!_channels) {
			if constexpr (std::is_same_v<T, std::uint8_t>)
				return GL_RED;
			else if constexpr (std::is_same_v<T, u8vec2>)
				return GL_RG;
			else if constexpr (std::is_same_v<T, u8vec3>)
				return GL_RGB;
			else if constexpr (std::is_same_v<T, u8vec4>)
				return GL_RGBA;
			else
				ASSERT("Invalid texture upload type");
			return GL_NONE;
		} else {
			switch (_channels) {
			case 1: return GL_RED;
			case 2: return GL_RG;
			case 3: return GL_RGB;
			case 4: return GL_RGBA;
			default:
				ASSERT("Invalid texture upload type");
				return GL_NONE;
			}
		}
	}();

	glBindTexture(GL_TEXTURE_2D, id);
	glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, size.x, size.y,
		channels, GL_UNSIGNED_BYTE, data);
}

template<PixelFmt F>
void Texture<F>::bind(TextureUnit const unit)
{
	ASSERT(id);

	glActiveTexture(+unit);
	glBindTexture(GL_TEXTURE_2D, id);
}

template<PixelFmt F>
void TextureMS<F>::create(char const* const _name, ivec2 const _size, Samples const _samples)
{
	ASSERT(!id);
	ASSERT(_name);
	ASSERT(Format != PixelFmt::None);
	ASSERT(+_samples >= 2);

	glGenTextures(1, &id);
#ifndef NDEBUG
	glObjectLabel(GL_TEXTURE, id, std::strlen(_name), _name);
#endif //NDEBUG
	name = _name;
	samples = _samples;
	resize(_size);

	L.debug(R"(Multisample texture "%s" created)", name);
}

template<PixelFmt F>
void TextureMS<F>::destroy()
{
#ifndef NDEBUG
	if (!id) {
		L.warn("Tried to destroy a multisample texture that has not been created");
		return;
	}
#endif //NDEBUG

	glDeleteTextures(1, &id);
	id = 0;
	size = {0, 0};
	samples = Samples::None;

	L.debug(R"(Multisample texture "%s" destroyed)", name);
	name = nullptr;
}

template<PixelFmt F>
void TextureMS<F>::resize(ivec2 const _size)
{
	ASSERT(_size.x > 0 && _size.y > 0);
	ASSERT(id);
	if (size == _size)
		return;

	glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, id);
	glTexImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, +samples, +Format,
		_size.x, _size.y, GL_TRUE);
	size = _size;
}

template<PixelFmt F>
void TextureMS<F>::bind(TextureUnit const unit)
{
	ASSERT(id);

	glActiveTexture(+unit);
	glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, id);
}

template<PixelFmt F>
void Renderbuffer<F>::create(char const* const _name, ivec2 const _size)
{
	ASSERT(!id);
	ASSERT(_name);
	ASSERT(Format != PixelFmt::None);

	glGenRenderbuffers(1, &id);
#ifndef NDEBUG
	glObjectLabel(GL_RENDERBUFFER, id, std::strlen(_name), _name);
#endif //NDEBUG
	name = _name;
	resize(_size);

	L.debug(R"(Renderbuffer "%s" created)", name);
}

template<PixelFmt F>
void Renderbuffer<F>::destroy()
{
#ifndef NDEBUG
	if (!id) {
		L.warn("Tried to destroy a renderbuffer that has not been created");
		return;
	}
#endif //NDEBUG

	glDeleteRenderbuffers(1, &id);
	id = 0;
	size = {0, 0};

	L.debug(R"(Renderbuffer "%s" destroyed)", name);
	name = nullptr;
}

template<PixelFmt F>
void Renderbuffer<F>::resize(ivec2 const _size)
{
	ASSERT(_size.x > 0 && _size.y > 0);
	ASSERT(id);
	if (size == _size)
		return;

	glBindRenderbuffer(GL_RENDERBUFFER, id);
	glRenderbufferStorage(GL_RENDERBUFFER, +Format, _size.x, _size.y);
	size = _size;
}

template<PixelFmt F>
void RenderbufferMS<F>::create(char const* const _name, ivec2 const _size, Samples const _samples)
{
	ASSERT(!id);
	ASSERT(_name);
	ASSERT(Format != PixelFmt::None);
	ASSERT(+_samples >= 2);

	glGenRenderbuffers(1, &id);
#ifndef NDEBUG
	glObjectLabel(GL_RENDERBUFFER, id, std::strlen(_name), _name);
#endif //NDEBUG
	name = _name;
	samples = _samples;
	resize(_size);

	L.debug(R"(Multisample renderbuffer "%s" created)", name);
}

template<PixelFmt F>
void RenderbufferMS<F>::destroy()
{
#ifndef NDEBUG
	if (!id) {
		L.warn("Tried to destroy a multisample renderbuffer that has not been created");
		return;
	}
#endif //NDEBUG

	glDeleteRenderbuffers(1, &id);
	id = 0;
	size = {0, 0};

	L.debug(R"(Multisample renderbuffer "%s" destroyed)", name);
	name = nullptr;
}

template<PixelFmt F>
void RenderbufferMS<F>::resize(ivec2 const _size)
{
	ASSERT(_size.x > 0 && _size.y > 0);
	ASSERT(id);
	if (size == _size)
		return;

	glBindRenderbuffer(GL_RENDERBUFFER, id);
	glRenderbufferStorageMultisample(GL_RENDERBUFFER, +samples, +Format,
		_size.x, _size.y);
	size = _size;
}

template<PixelFmt F>
void Framebuffer::attach(Texture<F>& t, Attachment const attachment)
{
	ASSERT(id);
	ASSERT(t.id);
	ASSERT(attachment != Attachment::None);
#ifndef NDEBUG
	if (t.Format == PixelFmt::DepthStencil)
		ASSERT(attachment == Attachment::DepthStencil);
	else
		ASSERT(attachment != Attachment::DepthStencil);
	if (samples != Samples::None)
		ASSERT(samples == Samples::_1);
	ASSERT(!detail::getAttachment(*this, attachment));
#endif //NDEBUG

	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, id);
	glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, +attachment,
		GL_TEXTURE_2D, t.id, 0);
	detail::getAttachment(*this, attachment) = &t;
	samples = Samples::_1;
	dirty = true;

	L.debug(R"(Texture "%s" attached to framebuffer "%s")", t.name, name);
}

template<PixelFmt F>
void Framebuffer::attach(TextureMS<F>& t, Attachment const attachment)
{
	ASSERT(id);
	ASSERT(t.id);
	ASSERT(attachment != Attachment::None);
#ifndef NDEBUG
	if (t.Format == PixelFmt::DepthStencil)
		ASSERT(attachment == Attachment::DepthStencil);
	else
		ASSERT(attachment != Attachment::DepthStencil);
	if (samples != Samples::None)
		ASSERT(samples == t.samples);
#endif //NDEBUG

	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, id);
	glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, +attachment,
		GL_TEXTURE_2D_MULTISAMPLE, t.id, 0);
	detail::getAttachment(*this, attachment) = &t;
	samples = t.samples;
	dirty = true;

	L.debug(R"(Multisample texture "%s" attached to framebuffer "%s")", t.name, name);
}

template<PixelFmt F>
void Framebuffer::attach(Renderbuffer<F>& r, Attachment const attachment)
{
	ASSERT(id);
	ASSERT(r.id);
	ASSERT(attachment != Attachment::None);
#ifndef NDEBUG
	if (r.Format == PixelFmt::DepthStencil)
		ASSERT(attachment == Attachment::DepthStencil);
	else
		ASSERT(attachment != Attachment::DepthStencil);
	if (samples != Samples::None)
		ASSERT(samples == Samples::_1);
#endif //NDEBUG

	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, id);
	glFramebufferRenderbuffer(GL_DRAW_FRAMEBUFFER, +attachment,
		GL_RENDERBUFFER, r.id);
	detail::getAttachment(*this, attachment) = &r;
	samples = Samples::_1;
	dirty = true;

	L.debug(R"(Renderbuffer "%s" attached to framebuffer "%s")", r.name, name);
}

template<PixelFmt F>
void Framebuffer::attach(RenderbufferMS<F>& r, Attachment const attachment)
{
	ASSERT(id);
	ASSERT(r.id);
	ASSERT(attachment != Attachment::None);
#ifndef NDEBUG
	if (r.Format == PixelFmt::DepthStencil)
		ASSERT(attachment == Attachment::DepthStencil);
	else
		ASSERT(attachment != Attachment::DepthStencil);
	if (samples != Samples::None)
		ASSERT(samples == r.samples);
#endif //NDEBUG

	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, id);
	glFramebufferRenderbuffer(GL_DRAW_FRAMEBUFFER, +attachment,
		GL_RENDERBUFFER, r.id);
	detail::getAttachment(*this, attachment) = &r;
	samples = r.samples;
	dirty = true;

	L.debug(R"(Multisample renderbuffer "%s" attached to framebuffer "%s")", r.name, name);
}

template<GLSLType T>
void Uniform<T>::setLocation(Shader const& shader, char const* const name)
{
	ASSERT(shader.id);
	ASSERT(name);

	location = glGetUniformLocation(shader.id, name);

	if (location == -1)
		L.warn(R"(Failed to get location for uniform "%s")", name);
}

template<GLSLType T>
void Uniform<T>::set(Type const val)
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

template<template<PixelFmt> typename T>
void Sampler<T>::setLocation(Shader const& shader, char const* const name, TextureUnit const _unit)
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

template<template<PixelFmt> typename T>
template<PixelFmt F>
void Sampler<T>::set(T<F>& val)
{
	val.bind(unit);
}

}
