// Minote - sys/opengl/texture.hpp
// Type-safe wrapper for OpenGL texture types

#pragma once

#include "glad/glad.h"
#include "base/util.hpp"
#include "base/math.hpp"
#include "sys/opengl/base.hpp"
#include "sys/opengl/buffer.hpp"

namespace minote {

// Texture filtering mode
enum struct Filter : GLint {
	None = GL_NONE,
	Nearest = GL_NEAREST,
	Linear = GL_LINEAR
};

// A texture upload format
template<typename T>
concept UploadFmt =
	std::is_same_v<T, u8> ||
	std::is_same_v<T, u8vec2> ||
	std::is_same_v<T, u8vec3> ||
	std::is_same_v<T, u8vec4>;

// Internal pixel format
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

// Number of samples per pixel
enum struct Samples : GLsizei {
	None = 0,
	_1 = 1,
	_2 = 2,
	_4 = 4,
	_8 = 8
};

// Index of a GPU texture unit
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

// Common fields of texture types
struct TextureBase : GLObject {

	// Size of the texture's storage. {0, 0} means no storage
	uvec2 size = {0, 0};

};

// Standard 2D texture, usable for reading and writing inside shaders
template<PixelFmt F>
struct Texture : TextureBase {

	// Internal pixel format
	static constexpr auto Format = F;

	// Active filtering mode
	Filter filter = Filter::None;

	// Initialize the texture object and allocate storage for it. Storage
	// contents are initially undefined. The default filtering mode is Linear.
	void create(char const* name, uvec2 size);

	// Destroy the OpenGL texture object and its storage.
	void destroy();

	// Set the filtering mode for the texture.
	void setFilter(Filter filter);

	// Recreate the texture's storage with new size. Previous contents are lost,
	// and the texture data is undefined again.
	void resize(uvec2 size);

	// Upload pixel data to texture storage. Each channel must be of type u8,
	// and the number of u8vec components decides the number of channels.
	// The array must have (size.x * size.y) elements.
	template<template<UploadFmt, size_t> typename Arr, UploadFmt T, size_t N>
		requires ArrayContainer<Arr, T, N>
	void upload(Arr<T, N> const& data);

	// The C-style array version. data length is expected to be
	// (size.x * size.y * channels) bytes long. If channels is 0, its value
	// is guessed from the uvec component count.
	template<UploadFmt T>
	void upload(T const data[], size_t dataLen, int channels = 0);

	// Bind the texture to the specified texture unit. This allows it to be used
	// in a shader for reading and/or writing. Unit None binds to the previously
	// selected unit.
	void bind(TextureUnit unit = TextureUnit::None);

};

// Multisample 2D texture, used in multisampled draws
template<PixelFmt F>
struct TextureMS : TextureBase {

	// Internal pixel format
	static constexpr auto Format = F;

	// Number of samples per pixel
	Samples samples = Samples::None;

	// Initialize the multisample texture object and allocate storage for it.
	// Storage contents are initially undefined.
	void create(char const* name, uvec2 size, Samples samples);

	// Destroy the OpenGL multisample texture object and its storage.
	void destroy();

	// Recreate the texture's storage with new size. Previous contents are lost,
	// and the texture data is undefined again.
	void resize(uvec2 size);

	// Bind the texture to the specified texture unit. This allows it to be used
	// in a shader for reading and/or writing. Unit None binds to the previously
	// selected unit.
	void bind(TextureUnit unit = TextureUnit::None);

};

// A texture type that can be read in a shader
template<typename T>
concept GLSLTexture =
	std::is_same_v<T, Texture<T::Format>> ||
	std::is_same_v<T, TextureMS<T::Format>>;

// Renderbuffer object. Operates faster than a texture, but cannot be read
template<PixelFmt F>
struct Renderbuffer : TextureBase {

	// Internal pixel format
	static constexpr auto Format = F;

	// Initialize the renderbuffer object and allocate storage for it.
	// Storage contents are initially undefined.
	void create(char const* name, uvec2 size);

	// Destroy the OpenGL renderbuffer object and its storage.
	void destroy();

	// Recreate the renderbuffer's storage with new size. Previous contents
	// are lost, and the renderbuffer data is undefined again.
	void resize(uvec2 size);

};

// Multisample renderbuffer object. Operates faster than a multisample texture,
// but cannot be read
template<PixelFmt F>
struct RenderbufferMS : TextureBase {

	// Internal pixel format
	static constexpr auto Format = F;

	// Number of samples per pixel
	Samples samples = Samples::None;

	// Initialize the multisample renderbuffer object and allocate storage
	// for it. Storage contents are initially undefined.
	void create(char const* name, uvec2 size, Samples samples);

	// Destroy the OpenGL multisample renderbuffer object and its storage.
	void destroy();

	// Recreate the multisample renderbuffer's storage with new size. Previous
	// contents are lost, and the renderbuffer data is undefined again.
	void resize(uvec2 size);

};

// A pixel format of a buffer texture
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

// Buffer texture object. Serves as a 1D texture that can only be read
// via texelFetch(). A buffer object is used as storage, and very large sizes
// are supported.
template<BufferTextureType T>
struct BufferTexture : TextureBase {

	// Format of the stored data
	using Type = T;

	// Buffer specialization for the buffer texture's internal storage
	using StorageBuffer = BufferBase<Type, GL_TEXTURE_BUFFER>;

	// Buffer object used as storage
	StorageBuffer storage;

	// Create the buffer texture; the storage is empty by default. Set dynamic
	// to true if you want to upload pixel data more than once (data streaming.)
	void create(char const* name, bool dynamic);

	// Destroy the OpenGL buffer texture object and its storage buffer.
	void destroy();

	// Upload new data to the texture, replacing previous data. The texture
	// is resized to fit the new data, and the previous storage is orphaned.
	// New size of the texture is (1, N). Pixel formats without a GLSL type
	// equivalent are normalized to f32.
	template<template<BufferTextureType, size_t> typename Arr, size_t N>
		requires ArrayContainer<Arr, Type, N>
	void upload(Arr<Type, N> data);

	// Bind the buffer texture to the specified texture unit. This allows it
	// to be used in a shader for reading and/or writing. Unit None binds
	// to the previously selected unit.
	void bind(TextureUnit unit = TextureUnit::None);

};

}

#include "sys/opengl/texture.tpp"
