// Minote - sys/opengl/shader.hpp
// Type-safe wrapper for OpenGL shader programs, uniforms and samplers

#pragma once

#include "glad/glad.h"
#include "base/util.hpp"
#include "sys/opengl/base.hpp"
#include "sys/opengl/texture.hpp"
#include "sys/opengl/buffer.hpp"

namespace minote {

struct Shader : GLObject {

	void create(char const* name, char const* vertSrc, char const* fragSrc);

	void destroy();

	void bind() const;

	virtual void setLocations() = 0;

};

template<typename T>
concept ShaderType = std::is_base_of_v<Shader, T>;

template<GLSLType T>
struct Uniform {

	using Type = T;

	GLint location = -1;
	GLuint shaderId = 0;
	Type value = {};

	void setLocation(Shader const& shader, char const* name);

	void set(Type value);

	inline auto operator=(Type val) -> Uniform<Type>& { set(val); return *this; }

	inline auto operator=(Uniform const& other) -> Uniform& {
		if (&other != this)
			set(other);
		return *this;
	}

	inline operator Type&() { return value; }
	inline operator Type() const { return value; }

	Uniform() = default;
	Uniform(Uniform const&) = delete;
	Uniform(Uniform&&) = delete;
	auto operator=(Uniform&&) -> Uniform& = delete;

};

template<template<PixelFmt> typename T>
struct Sampler {

	GLint location = -1;
	TextureUnit unit = TextureUnit::None;

	void setLocation(Shader const& shader, char const* name, TextureUnit unit = TextureUnit::_0);

	template<PixelFmt F>
	void set(T<F>& val);

	template<PixelFmt F>
	inline auto operator=(T<F>& val) -> Sampler<T>& { set(val); return *this; }

	Sampler() = default;
	Sampler(Sampler const&) = delete;
	Sampler(Sampler&&) = delete;
	inline auto operator=(Sampler const& other) -> Sampler& = delete;
	auto operator=(Sampler&&) -> Sampler& = delete;

};

struct BufferSampler {

	GLint location = -1;
	TextureUnit unit = TextureUnit::None;

	void setLocation(Shader const& shader, char const* name, TextureUnit unit = TextureUnit::_0);

	template<BufferTextureType T>
	void set(BufferTexture<T>& val);

	template<BufferTextureType T>
	inline auto operator=(BufferTexture<T>& val) -> BufferSampler& { set(val); return *this; }

	BufferSampler() = default;
	BufferSampler(BufferSampler const&) = delete;
	BufferSampler(BufferSampler&&) = delete;
	inline auto operator=(BufferSampler const& other) -> BufferSampler& = delete;
	auto operator=(BufferSampler&&) -> BufferSampler& = delete;

};

}

#include "sys/opengl/shader.tpp"
