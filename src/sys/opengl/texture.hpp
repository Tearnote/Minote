// Minote - sys/opengl/texture.hpp
// Type-safe wrapper for OpenGL texture types

#pragma once

#include "glad/glad.h"
#include "base/util.hpp"
#include "base/math.hpp"
#include "sys/opengl/base.hpp"
#include "sys/opengl/buffer.hpp"

namespace minote {

// Valid texture upload format
template<typename T>
concept UploadFmt =
	std::is_same_v<T, u8> ||
	std::is_same_v<T, u8vec2> ||
	std::is_same_v<T, u8vec3> ||
	std::is_same_v<T, u8vec4>;

/// Available texture filtering modes
enum struct Filter : GLint {
	None = GL_NONE,
	Nearest = GL_NEAREST,
	Linear = GL_LINEAR
};

/// Available internal pixel formats
enum struct PixelFmt : GLint {
	None = GL_NONE,
	R_u8 = GL_R8,
	RG_u8 = GL_RG8,
	RGBA_u8 = GL_RGBA8,
	R_f16 = GL_R16F,
	RG_f16 = GL_RG16F,
	RGBA_f16 = GL_RGBA16F,
	DepthStencil = GL_DEPTH24_STENCIL8
};

enum struct Samples : GLsizei {
	None = 0,
	_1 = 1,
	_2 = 2,
	_4 = 4,
	_8 = 8
};

enum struct TextureUnit : GLenum {
	None = 0,
	_0 = GL_TEXTURE0,
	_1 = GL_TEXTURE1,
	_2 = GL_TEXTURE2,
	_3 = GL_TEXTURE3,
	_4 = GL_TEXTURE4,
	_5 = GL_TEXTURE5,
	_6 = GL_TEXTURE6,
	_7 = GL_TEXTURE7,
	_8 = GL_TEXTURE8,
	_9 = GL_TEXTURE9,
	_10 = GL_TEXTURE10,
	_11 = GL_TEXTURE11,
	_12 = GL_TEXTURE12,
	_13 = GL_TEXTURE13,
	_14 = GL_TEXTURE14,
	_15 = GL_TEXTURE15,
};

/// Common fields of texture types
struct TextureBase : GLObject {

	uvec2 size = {0, 0}; ///< The texture does not have storage if this is {0, 0}

};

/// Standard 2D texture, usable for reading and writing inside shaders
template<PixelFmt F>
struct Texture : TextureBase {

	static constexpr auto Format = F;

	Filter filter = Filter::None;

	/**
	 * Create an OpenGL ID for the texture. This needs to be called before
	 * the texture can be used. Storage is allocated by default, and filled
	 * with garbage data. The default filtering mode is Linear.
	 * @param name Human-readable name, for logging and debug output
	 * @param size Initial size of the texture storage, in pixels
	 */
	void create(char const* name, uvec2 size);

	/**
	 * Destroy the OpenGL texture object. Storage and ID are both freed.
	 */
	void destroy();

	/**
	 * Set the filtering mode for the texture.
	 * @param filter New filtering mode
	 */
	void setFilter(Filter filter);

	/**
	 * Recreate the texture's storage with new size. Previous contents are lost,
	 * and the texture data is garbage again.
	 * @param size New size of the texture storage, in pixels
	 */
	void resize(uvec2 size);

	template<template<UploadFmt, size_t> typename Arr, UploadFmt T, size_t N>
	requires ArrayContainer<Arr, T, N>
	void upload(Arr<T, N> const& data);

	template<UploadFmt T>
	void upload(T const data[], int channels = 0); //TODO make safe

	/**
	 * Bind the texture to the specified texture unit. This allows it to be used
	 * in a shader for reading and/or writing.
	 * @param unit Texture unit to bind the texture to
	 */
	void bind(TextureUnit unit = TextureUnit::None);

};

/// OpenGL multisample 2D texture. Allows for drawing antialiased shapes
template<PixelFmt F>
struct TextureMS : TextureBase {

	static constexpr auto Format = F;

	Samples samples = Samples::None;

	/**
	 * Create an OpenGL ID for the multisample texture. This needs to be called
	 * before the texture can be used. Storage is allocated by default,
	 * and filled with garbage data.
	 * @param name Human-readable name, for logging and debug output
	 * @param size Initial size of the multisample texture storage, in pixels
	 * @param samples Number of samples per pixel
	 */
	void create(char const* name, uvec2 size, Samples samples);

	/**
	 * Destroy the OpenGL multisample texture object. Storage and ID are both
	 * freed.
	 */
	void destroy();

	/**
	 * Recreate the multisample texture's storage with new size. Previous
	 * contents are lost, and the texture data is garbage again.
	 * @param size New size of the multisample texture storage, in pixels
	 */
	void resize(uvec2 size);

	/**
	 * Bind the multisample texture to the specified texture unit. This allows
	 * it to be used in a shader for reading and/or writing.
	 * @param unit Texture unit to bind the multisample texture to, from 0 to 15
	 * inclusive
	 */
	void bind(TextureUnit unit = TextureUnit::None);

};

/// Concept for any texture type that can be read in a shader
template<typename T>
concept GLSLTexture =
std::is_same_v<T, Texture<T::Format>> ||
	std::is_same_v<T, TextureMS<T::Format>>;

/// OpenGL renderbuffer. Operates faster than a texture, but cannot be read
template<PixelFmt F>
struct Renderbuffer : TextureBase {

	static constexpr auto Format = F;

	/**
	 * Create an OpenGL ID for the renderbuffer. This needs to be called before
	 * the renderbuffer can be used. Storage is allocated by default, and filled
	 * with garbage data.
	 * @param name Human-readable name, for logging and debug output
	 * @param size Initial size of the renderbuffer storage, in pixels
	 */
	void create(char const* name, uvec2 size);

	/**
	 * Destroy the OpenGL renderbuffer object. Storage and ID are both freed.
	 */
	void destroy();

	/**
	 * Recreate the renderbuffer's storage with new size. Previous contents
	 * are lost, and the renderbuffer data is garbage again.
	 * @param size New size of the renderbuffer storage, in pixels
	 */
	void resize(uvec2 size);

};

/// OpenGL multisample renderbuffer. Operates faster than a multisample texture,
/// but cannot be read
template<PixelFmt F>
struct RenderbufferMS : TextureBase {

	static constexpr auto Format = F;

	Samples samples = Samples::None;

	/**
	 * Create an OpenGL ID for the multisample renderbuffer. This needs
	 * to be called before the renderbuffer can be used. Storage is allocated
	 * by default, and filled with garbage data.
	 * @param name Human-readable name, for logging and debug output
	 * @param size Initial size of the multisample renderbuffer storage,
	 * in pixels
	 * @param samples Number of samples per pixel: 2, 4 or 8
	 */
	void create(char const* name, uvec2 size, Samples samples);

	/**
	 * Destroy the OpenGL multisample renderbuffer object. Storage and ID
	 * are both freed.
	 */
	void destroy();

	/**
	 * Recreate the multisample renderbuffer's storage with new size. Previous
	 * contents are lost, and the renderbuffer data is garbage again.
	 * @param size New size of the multisample renderbuffer storage, in pixels
	 */
	void resize(uvec2 size);

};

template<typename T>
concept BufferTextureType =
	std::is_same_v<T, f32> ||
	std::is_same_v<T, vec2> ||
	std::is_same_v<T, vec4> ||
	std::is_same_v<T, u8> ||
	std::is_same_v<T, u8vec2> ||
	std::is_same_v<T, u8vec4> ||
	std::is_same_v<T, u32> ||
	std::is_same_v<T, uvec2> ||
	std::is_same_v<T, uvec4> ||
	std::is_same_v<T, i32> ||
	std::is_same_v<T, ivec2> ||
	std::is_same_v<T, ivec4> ||
	std::is_same_v<T, mat4>;

template<BufferTextureType T>
struct BufferTexture : TextureBase {

	using Type = T;
	using StorageBuffer = BufferBase<Type, GL_TEXTURE_BUFFER>;

	StorageBuffer storage;

	void create(char const* name, bool dynamic);

	void destroy();

	template<template<BufferTextureType, size_t> typename Arr, size_t N>
	requires ArrayContainer<Arr, Type, N>
	void upload(Arr<Type, N> data);

	void bind(TextureUnit unit = TextureUnit::None);

};

}

#include "sys/opengl/texture.tpp"
