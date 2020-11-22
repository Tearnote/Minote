// Minote - sys/opengl/shader.hpp
// Type-safe wrapper for OpenGL shader programs and their uniforms and samplers

#pragma once

#include "glad/glad.h"
#include "base/util.hpp"
#include "sys/opengl/base.hpp"
#include "sys/opengl/texture.hpp"
#include "sys/opengl/buffer.hpp"

namespace minote {

// Shader program wrapper. To use, derive from the struct and add the shader's
// Uniforms and Samplers
struct Shader : GLObject {

	// Create, compile and link the shader program from source strings.
	void create(char const* name, char const* vertSrc, char const* fragSrc);

	// Destroy the shader program to free the resources.
	void destroy();

	// Bind the shader program to OpenGL state, causing all future draws
	// to invoke this shader.
	void bind() const;

	// This function will be called within create(). Override it to initialize
	// Uniforms and Samplers with setLocation() calls.
	virtual void setLocations() = 0;

};

// Any type that is a Shader
template<typename T>
concept ShaderType = std::is_base_of_v<Shader, T>;

// Shader uniform wrapper. Supports easy assignment and caches the last value
template<GLSLType T>
struct Uniform {

	// Value type managed by the uniform
	using Type = T;

	// Internal OpenGL uniform location
	GLint location = -1;

	// Internal ID of the related shader program
	GLuint shaderId = 0;

	// Last held value, used to minimize OpenGL calls
	Type value = {};

	// Initialize the uniform from a compiled shader. If the uniform location
	// is not found, the error is logged and all later use will silently fail.
	void setLocation(Shader const& shader, char const* name);

	// Set the uniform to a new value.
	void set(Type value);

	// Set value through assignment
	inline auto operator=(Type val) -> Uniform<Type>& { set(val); return *this; }

	// Support for chained assignment
	inline auto operator=(Uniform const& other) -> Uniform& {
		if (&other != this)
			set(other);
		return *this;
	}

	// Value read from cache, through conversion
	inline operator Type&() { return value; }
	inline operator Type() const { return value; }

	Uniform() = default;
	Uniform(Uniform const&) = delete;
	Uniform(Uniform&&) = delete;
	auto operator=(Uniform&&) -> Uniform& = delete;

};

// Shader sampler wrapper. Supports easy assignment
template<template<PixelFmt> typename T>
struct Sampler {

	// Internal OpenGL sampler location
	GLint location = -1;

	// Texture unit in use by the sampler
	TextureUnit unit = TextureUnit::None;

	// Initialize the sampler from a compiled shader. If the sampler location
	// is not found, the error is logged and all later use will silently fail.
	// Make sure to initialize every sampler of a shader to a different unit.
	void setLocation(Shader const& shader, char const* name, TextureUnit unit = TextureUnit::_0);

	// Bind a new texture to the sampler.
	template<PixelFmt F>
	void set(T<F>& val);

	// Set texture through assignment
	template<PixelFmt F>
	inline auto operator=(T<F>& val) -> Sampler<T>& { set(val); return *this; }

	Sampler() = default;
	Sampler(Sampler const&) = delete;
	Sampler(Sampler&&) = delete;
	inline auto operator=(Sampler const& other) -> Sampler& = delete;
	auto operator=(Sampler&&) -> Sampler& = delete;

};

// Buffer texture sampler wrapper. Supports easy assignment
struct BufferSampler {

	// Internal OpenGL sampler location
	GLint location = -1;

	// Texture unit in use by the sampler
	TextureUnit unit = TextureUnit::None;

	// Initialize the sampler from a compiled shader. If the sampler location
	// is not found, the error is logged and all later use will silently fail.
	// Make sure to initialize every sampler of a shader to a different unit.
	void setLocation(Shader const& shader, char const* name, TextureUnit unit = TextureUnit::_0);

	// Bind a new buffer texture to the sampler.
	template<BufferTextureType T>
	void set(BufferTexture<T>& val);

	// Set buffer texture through assignment
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
