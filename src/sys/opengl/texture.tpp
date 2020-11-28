#pragma once

#include <cstring>
#include "base/log.hpp"
#include "sys/opengl/state.hpp"

namespace minote {

template<PixelFmt F>
void Texture<F>::create(char const* const _name, uvec2 const _size)
{
	ASSERT(!id);
	ASSERT(_name);
	ASSERT(Format != PixelFmt::None);

	glGenTextures(1, &id);
#ifndef NDEBUG
	glObjectLabel(GL_TEXTURE, id, std::strlen(_name), _name);
#endif //NDEBUG
	name = _name;
	bind();
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	setFilter(Filter::Linear);
	resize(_size);

	L.debug(R"(Texture "{}" created)", name);
}

template<PixelFmt F>
void Texture<F>::destroy()
{
	ASSERT(id);

	detail::state.deleteTexture(GL_TEXTURE_2D, id);
	id = 0;
	size = {0, 0};
	filter = Filter::None;

	L.debug(R"(Texture "{}" destroyed)", name);
	name = nullptr;
}

template<PixelFmt F>
void Texture<F>::setFilter(Filter const _filter)
{
	ASSERT(_filter != Filter::None);
	if (filter == _filter)
		return;

	bind();
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, +_filter);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, +_filter);
	filter = _filter;
}

template<PixelFmt F>
void Texture<F>::resize(uvec2 const _size)
{
	ASSERT(_size.x > 0 && _size.y > 0);
	ASSERT(id);
	if (size == _size)
		return;

	bind();
	glTexImage2D(GL_TEXTURE_2D, 0, +Format,
		_size.x, _size.y, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
	size = _size;
}

template<PixelFmt F>
template<template<UploadFmt, size_t> typename Arr, UploadFmt T, size_t N>
requires ArrayContainer<Arr, T, N>
void Texture<F>::upload(Arr<T, N> const& data)
{
	ASSERT(data.size() == size.x * size.y);
	ASSERT(id);
	ASSERT(size.x > 0 && size.y > 0);
	ASSERT(Format != PixelFmt::DepthStencil);

	constexpr GLenum channels = [] {
		if constexpr (std::is_same_v<T, u8>)
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

	bind();
	glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, size.x, size.y,
		channels, GL_UNSIGNED_BYTE, data.data());
}

template<PixelFmt F>
template<UploadFmt T>
void Texture<F>::upload(T const data[], size_t const dataLen, int const _channels)
{
	ASSERT(data);
	ASSERT(_channels >= 0 && _channels <= 4);
	ASSERT(id);
	ASSERT(size.x > 0 && size.y > 0);
	ASSERT(Format != PixelFmt::DepthStencil);

	const GLenum channels = [=] {
		if (!_channels) {
			if constexpr (std::is_same_v<T, u8>)
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

	ASSERT(dataLen == size.x * size.y * _channels);

	bind();
	glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, size.x, size.y,
		channels, GL_UNSIGNED_BYTE, data);
}

template<PixelFmt F>
void Texture<F>::bind(TextureUnit const unit)
{
	ASSERT(id);

	detail::state.setTextureUnit(+unit);
	detail::state.bindTexture(GL_TEXTURE_2D, id);
}

template<PixelFmt F>
void TextureMS<F>::create(char const* const _name, uvec2 const _size, Samples const _samples)
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

	L.debug(R"(Multisample texture "{}" created)", name);
}

template<PixelFmt F>
void TextureMS<F>::destroy()
{
	ASSERT(id);

	detail::state.deleteTexture(GL_TEXTURE_2D_MULTISAMPLE, id);
	id = 0;
	size = {0, 0};
	samples = Samples::None;

	L.debug(R"(Multisample texture "{}" destroyed)", name);
	name = nullptr;
}

template<PixelFmt F>
void TextureMS<F>::resize(uvec2 const _size)
{
	ASSERT(_size.x > 0 && _size.y > 0);
	ASSERT(id);
	if (size == _size)
		return;

	bind();
	glTexImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, +samples, +Format,
		_size.x, _size.y, GL_TRUE);
	size = _size;
}

template<PixelFmt F>
void TextureMS<F>::bind(TextureUnit const unit)
{
	ASSERT(id);

	detail::state.setTextureUnit(+unit);
	detail::state.bindTexture(GL_TEXTURE_2D_MULTISAMPLE, id);
}

template<PixelFmt F>
void Renderbuffer<F>::create(char const* const _name, uvec2 const _size)
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

	L.debug(R"(Renderbuffer "{}" created)", name);
}

template<PixelFmt F>
void Renderbuffer<F>::destroy()
{
	ASSERT(id);

	detail::state.deleteRenderbuffer(id);
	id = 0;
	size = {0, 0};

	L.debug(R"(Renderbuffer "{}" destroyed)", name);
	name = nullptr;
}

template<PixelFmt F>
void Renderbuffer<F>::resize(uvec2 const _size)
{
	ASSERT(_size.x > 0 && _size.y > 0);
	ASSERT(id);
	if (size == _size)
		return;

	detail::state.bindRenderbuffer(id);
	glRenderbufferStorage(GL_RENDERBUFFER, +Format, _size.x, _size.y);
	size = _size;
}

template<PixelFmt F>
void RenderbufferMS<F>::create(char const* const _name, uvec2 const _size, Samples const _samples)
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

	L.debug(R"(Multisample renderbuffer "{}" created)", name);
}

template<PixelFmt F>
void RenderbufferMS<F>::destroy()
{
	ASSERT(id);

	detail::state.deleteRenderbuffer(id);
	id = 0;
	size = {0, 0};

	L.debug(R"(Multisample renderbuffer "{}" destroyed)", name);
	name = nullptr;
}

template<PixelFmt F>
void RenderbufferMS<F>::resize(uvec2 const _size)
{
	ASSERT(_size.x > 0 && _size.y > 0);
	ASSERT(id);
	if (size == _size)
		return;

	detail::state.bindRenderbuffer(id);
	glRenderbufferStorageMultisample(GL_RENDERBUFFER, +samples, +Format,
		_size.x, _size.y);
	size = _size;
}

template<BufferTextureType T>
void BufferTexture<T>::create(char const* const _name, bool const dynamic)
{
	ASSERT(!id);
	ASSERT(_name);

	glGenTextures(1, &id);
#ifndef NDEBUG
	glObjectLabel(GL_TEXTURE, id, std::strlen(_name), _name);
#endif //NDEBUG
	name = _name;

	constexpr auto format = []() -> GLenum {
		if constexpr (std::is_same_v<T, f32>)
			return GL_R32F;
		if constexpr (std::is_same_v<T, vec2>)
			return GL_RG32F;
		if constexpr (std::is_same_v<T, vec4>)
			return GL_RGBA32F;
		if constexpr (std::is_same_v<T, u8>)
			return GL_R8;
		if constexpr (std::is_same_v<T, u8vec2>)
			return GL_RG8;
		if constexpr (std::is_same_v<T, u8vec4>)
			return GL_RGBA8;
		if constexpr (std::is_same_v<T, u32>)
			return GL_R32UI;
		if constexpr (std::is_same_v<T, uvec2>)
			return GL_RG32UI;
		if constexpr (std::is_same_v<T, uvec4>)
			return GL_RGBA32UI;
		if constexpr (std::is_same_v<T, i32>)
			return GL_R32I;
		if constexpr (std::is_same_v<T, ivec2>)
			return GL_RG32I;
		if constexpr (std::is_same_v<T, ivec4>)
			return GL_RGBA32I;
		if constexpr (std::is_same_v<T, mat4>)
			return GL_RGBA32F;
		L.fail("Unknown buffer texture type");
	}();

	storage.create(_name, dynamic);
	storage.bind();
	glBufferData(StorageBuffer::Target, 0, nullptr,
		dynamic? GL_STREAM_DRAW : GL_STATIC_DRAW);
	bind(TextureUnit::_0);
	glTexBuffer(GL_TEXTURE_BUFFER, format, storage.id);

	L.debug(R"(Buffer texture "{}" created)", name);
}

template<BufferTextureType T>
void BufferTexture<T>::destroy()
{
	ASSERT(id);

	detail::state.deleteTexture(GL_TEXTURE_BUFFER, id);
	id = 0;
	size = {0, 0};
	storage.destroy();

	L.debug(R"(Buffer texture "{}" destroyed)", name);
	name = nullptr;
}

template<BufferTextureType T>
template<template<BufferTextureType, size_t> typename Arr, size_t N>
requires ArrayContainer<Arr, T, N>
void BufferTexture<T>::upload(Arr<T, N> data)
{
	storage.upload(data);
	size = {N, 1};
}

template<BufferTextureType T>
void BufferTexture<T>::bind(TextureUnit unit)
{
	ASSERT(id);

	detail::state.setTextureUnit(+unit);
	detail::state.bindTexture(GL_TEXTURE_BUFFER, id);
}

}
