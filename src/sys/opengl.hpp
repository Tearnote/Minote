/**
 * A thin OpenGL wrapper for dealing with most common objects
 * @file
 */

#pragma once

#include "glad/glad.h"
#include "base/array.hpp"
#include "base/math.hpp"
#include "sys/window.hpp"

namespace minote {

/// A type that has an equivalent in GLSL
template<typename T>
concept GLSLType =
	std::is_same_v<T, f32> ||
	std::is_same_v<T, u32> ||
	std::is_same_v<T, i32> ||
	std::is_same_v<T, vec2> ||
	std::is_same_v<T, vec3> ||
	std::is_same_v<T, vec4> ||
	std::is_same_v<T, uvec2> ||
	std::is_same_v<T, uvec3> ||
	std::is_same_v<T, uvec4> ||
	std::is_same_v<T, ivec2> ||
	std::is_same_v<T, ivec3> ||
	std::is_same_v<T, ivec4> ||
	std::is_same_v<T, mat4>;

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

template<typename T>
concept UploadFmt =
	std::is_same_v<T, u8> ||
	std::is_same_v<T, u8vec2> ||
	std::is_same_v<T, u8vec3> ||
	std::is_same_v<T, u8vec4>;

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

enum struct Samples : GLsizei {
	None = 0,
	_1 = 1,
	_2 = 2,
	_4 = 4,
	_8 = 8
};

enum struct Attachment : GLenum {
	None = GL_NONE,
	DepthStencil = GL_DEPTH_STENCIL_ATTACHMENT,
	Color0 = GL_COLOR_ATTACHMENT0,
	Color1 = GL_COLOR_ATTACHMENT1,
	Color2 = GL_COLOR_ATTACHMENT2,
	Color3 = GL_COLOR_ATTACHMENT3,
	Color4 = GL_COLOR_ATTACHMENT4,
	Color5 = GL_COLOR_ATTACHMENT5,
	Color6 = GL_COLOR_ATTACHMENT6,
	Color7 = GL_COLOR_ATTACHMENT7,
	Color8 = GL_COLOR_ATTACHMENT8,
	Color9 = GL_COLOR_ATTACHMENT9,
	Color10 = GL_COLOR_ATTACHMENT10,
	Color11 = GL_COLOR_ATTACHMENT11,
	Color12 = GL_COLOR_ATTACHMENT12,
	Color13 = GL_COLOR_ATTACHMENT13,
	Color14 = GL_COLOR_ATTACHMENT14,
	Color15 = GL_COLOR_ATTACHMENT15,
};

/// Common fields of all OpenGL object types
struct GLObject {

	GLuint id = 0; ///< The object has not been created if this is 0
	char const* name = nullptr; ///< Human-readable name, used in logging

	~GLObject();
};

template<TriviallyCopyable T, GLenum _target>
struct BufferBase : GLObject {

	using Type = T;
	static constexpr GLenum Target = _target;

	bool dynamic = false;

	bool uploaded = false;

	void create(char const* name, bool dynamic);

	void destroy();

	template<template<TriviallyCopyable, size_t> typename Arr, size_t N>
		requires ArrayContainer<Arr, Type, N>
	void upload(Arr<Type, N> const& data);

	void upload(size_t elements, Type data[]); //TODO remove

	void bind() const;

};

template<TriviallyCopyable T>
using VertexBuffer = BufferBase<T, GL_ARRAY_BUFFER>;

template<typename T>
concept ElementType =
	std::is_same_v<T, u8> ||
	std::is_same_v<T, u16> ||
	std::is_same_v<T, u32>;

using ElementBuffer = BufferBase<u32, GL_ELEMENT_ARRAY_BUFFER>;

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

struct VertexArray : GLObject {

	array<bool, 16> attributes = {};
	bool elements = false;

	void create(char const* name);

	void destroy();

	template<GLSLType T>
	void setAttribute(GLuint index, VertexBuffer<T>& buffer, bool instanced = false);

	template<TriviallyCopyable T, GLSLType U>
	void setAttribute(GLuint index, VertexBuffer<T>& buffer, U T::*field, bool instanced = false);

	void setElements(ElementBuffer& buffer);

	void bind();

};

/// OpenGL framebuffer. Proxy object that allows drawing into textures
/// and renderbuffers from within shaders.
struct Framebuffer : GLObject {

	Samples samples = Samples::None; ///< Sample count of all attachments needs to match
	bool dirty = true; ///< Is a glDrawBuffers call and completeness check needed?
	array<TextureBase const*, 17> attachments = {};

	/**
	 * Create an OpenGL ID for the framebuffer. This needs to be called before
	 * the framebuffer can be used. The object has no textures attached
	 * by default, and needs to have at least one color attachment attached
	 * to satisfy completeness requirements.
	 * @param name Human-readable name, for logging and debug output
	 */
	void create(char const* name);

	/**
	 * Destroy the OpenGL framebuffer object. The ID is released, but attached
	 * objects continue to exist.
	 */
	void destroy();

	/**
	 * Attach a texture to a specified attachment point. All future attachments
	 * must not be multisample. The DepthStencil attachment can only hold
	 * a texture with a DepthStencil pixel format. Attachments cannot
	 * be overwritten.
	 * @param t Texture to attach
	 * @param attachment Attachment point to attach to
	 */
	template<PixelFmt F>
	void attach(Texture<F>& t, Attachment attachment);

	/**
	 * Attach a multisample texture to a specified attachment point. All future
	 * attachments must have the same number of samples. The DepthStencil
	 * attachment can only hold a texture with a DepthStencil pixel format.
	 * Attachments cannot be overwritten.
	 * @param t Multisample texture to attach
	 * @param attachment Attachment point to attach to
	 */
	template<PixelFmt F>
	void attach(TextureMS<F>& t, Attachment attachment);

	/**
	 * Attach a renderbuffer to a specified attachment point. All future
	 * attachments must not be multisample. The DepthStencil attachment can only
	 * hold a renderbuffer with a DepthStencil pixel format. Attachments
	 * cannot be overwritten.
	 * @param t Renderbuffer to attach
	 * @param attachment Attachment point to attach to
	 */
	template<PixelFmt F>
	void attach(Renderbuffer<F>& r, Attachment attachment);

	/**
	 * Attach a multisample renderbuffer to a specified attachment point.
	 * All future attachments must have the same number of samples.
	 * The DepthStencil attachment can only hold a renderbuffer
	 * with a DepthStencil pixel format. Attachments cannot be overwritten.
	 * @param t Multisample renderbuffer to attach
	 * @param attachment Attachment point to attach to
	 */
	template<PixelFmt F>
	void attach(RenderbufferMS<F>& r, Attachment attachment);

	auto size() -> uvec2;

	/**
	 * Bind this framebuffer to the OpenGL context, causing all future draw
	 * commands to render into the framebuffer's attachments. In a debug build,
	 * the framebuffer is checked for completeness.
	 */
	void bind();

	void bindRead() const;

	/**
	 * Bind the zero framebuffer, which causes all future draw commands to draw
	 * to the window surface.
	 */
	static void unbind();

	/**
	 * Copy the contents of one framebuffer to another. MSAA resolve
	 * is performed if required.
	 * @param dst Destination framebuffer
	 * @param src Source framebuffer
	 * @param srcBuffer The attachment in src to read from
	 * @param depthStencil Whether to blit the DepthStencil attachment
	 */
	static void blit(Framebuffer& dst, Framebuffer const& src,
		Attachment srcBuffer = Attachment::Color0, bool depthStencil = false);

};

struct Shader : GLObject {

	void create(char const* name, char const* vertSrc, char const* fragSrc);

	void destroy();

	void bind() const;

	virtual void setLocations() = 0;

};

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

enum struct DrawMode : GLenum {
	Triangles = GL_TRIANGLES,
	TriangleStrip = GL_TRIANGLE_STRIP
};

enum struct Comparison : GLenum {
	Never = GL_NEVER,
	Always = GL_ALWAYS,
	Equal = GL_EQUAL,
	Inequal = GL_NOTEQUAL,
	Lesser = GL_LESS,
	Greater = GL_GREATER,
	LesserEqual = GL_LEQUAL,
	GreaterEqual = GL_GEQUAL
};

enum struct BlendingOp : GLenum {
	Zero = GL_ZERO,
	One = GL_ONE,
	SrcAlpha = GL_SRC_ALPHA,
	OneMinusSrcAlpha = GL_ONE_MINUS_SRC_ALPHA
};

enum struct StencilOp : GLenum {
	Nothing = GL_KEEP,
	Clear = GL_ZERO,
	Set = GL_REPLACE,
	Increment = GL_INCR,
	Decrement = GL_DECR,
	Invert = GL_INVERT
};

struct DrawParams {

	bool blending = false;
	struct BlendingMode {
		BlendingOp src;
		BlendingOp dst;
	} blendingMode = { BlendingOp::SrcAlpha, BlendingOp::OneMinusSrcAlpha };

	bool culling = true;

	bool depthTesting = true;
	Comparison depthFunc = Comparison::LesserEqual;

	bool scissorTesting = false;
	AABB<2, i32> scissorBox;

	bool stencilTesting = false;
	struct StencilMode {
		Comparison func = Comparison::Equal;
		i32 ref = 0;
		StencilOp sfail = StencilOp::Nothing;
		StencilOp dpfail = StencilOp::Nothing;
		StencilOp dppass = StencilOp::Nothing;
	} stencilMode;

	AABB<2, i32> viewport;

	bool colorWrite = true;

	void set() const;

};

template<typename T>
concept ShaderType = std::is_base_of_v<Shader, T>;

template<ShaderType T>
struct Draw {

	T* shader = nullptr;
	VertexArray* vertexarray = nullptr;
	Framebuffer* framebuffer = nullptr;
	DrawMode mode = DrawMode::Triangles;
	GLsizei triangles = 0;
	GLsizei instances = 1;
	GLint offset = 0;
	DrawParams params;

	void draw(Window* window = nullptr);

};

}

#include "sys/opengl.tpp"
