/**
 * A thin OpenGL wrapper for dealing with most common objects
 * @file
 */

#pragma once

#include <cstddef>
#include "glad/glad.h"
#include "base/math.hpp"
#include "sys/window.hpp"

namespace minote {

/// OpenGL shader program. You can obtain an instance with programCreate().
using Program = GLuint;

/// OpenGL uniform location. This is used as part of a struct containing ::ProgramBase.
using Uniform = GLint;

/// OpenGL texture unit. This is used as part of a struct containing ::ProgramBase.
using TextureUnit = GLenum;

/// OpenGL vertex buffer object ID
using VertexBuffer = GLuint;

/// OpenGL vertex array object ID
using VertexArray = GLuint;

/// OpenGL element array object ID
using ElementArray = GLuint;

/// OpenGL buffer texture object ID
using BufferTexture = GLuint;

/// OpenGL buffer object, for use with buffer textures
using BufferTextureStorage = GLuint;

/// Available texture filtering modes
enum struct Filter : GLint {
	None = GL_NONE,
	Nearest = GL_NEAREST,
	Linear = GL_LINEAR
};

/// Available internal pixel formats
enum struct PixelFormat : GLint {
	None = GL_NONE,
	R_u8 = GL_R8,
	RG_u8 = GL_RG8,
	RGB_u8 = GL_RGB8,
	RGBA_u8 = GL_RGBA8,
	R_f16 = GL_R16F,
	RG_f16 = GL_RG16F,
	RGB_f16 = GL_RGB16F,
	RGBA_f16 = GL_RGBA16F,
	DepthStencil = GL_DEPTH24_STENCIL8
};

/// Common fields of all OpenGL object types
struct GLObject {

	GLuint id = 0; ///< The object has not been created if this is 0
	const char* name = nullptr; ///< Human-readable name, used in logging

	~GLObject();
};

/// Common fields of texture types
struct TextureBase : GLObject {

	ivec2 size = {0, 0}; ///< The texture does not have storage if this is {0, 0}
	PixelFormat format = PixelFormat::None;

};

/// Standard 2D texture, usable for reading and writing inside shaders
struct Texture : TextureBase {

	Filter filter = Filter::None;

	/**
	 * Create an OpenGL ID for the texture. This needs to be called before
	 * the texture can be used. Storage is allocated by default, and filled
	 * with garbage data. The default filtering mode is Linear.
	 * @param name Human-readable name, for logging and debug output
	 * @param size Initial size of the texture storage, in pixels
	 * @param format Internal format of the texture
	 */
	void create(const char* name, ivec2 size, PixelFormat format);

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
	void resize(ivec2 size);

	/**
	 * Upload texture data from CPU to the texture object, replacing previous
	 * contents. Expected pixel format is 1 byte per channel (0-255),
	 * same number of channels as internal format, and size.x * size.y pixels.
	 * Uploading to a stencil+depth texture is not supported.
	 * @param data Pixel data as an unchecked array of bytes
	 */
	void upload(std::uint8_t* data);

	/**
	 * Bind the texture to the specified texture unit. This allows it to be used
	 * in a shader for reading and/or writing.
	 * @param unit Texture unit to bind the texture to
	 */
	void bind(TextureUnit unit);

};

/// OpenGL multisample 2D texture. Allows for drawing antialiased shapes
struct TextureMS : TextureBase {

	GLsizei samples = 0;

	/**
	 * Create an OpenGL ID for the multisample texture. This needs to be called
	 * before the texture can be used. Storage is allocated by default,
	 * and filled with garbage data.
	 * @param name Human-readable name, for logging and debug output
	 * @param size Initial size of the multisample texture storage, in pixels
	 * @param format Internal format of the multisample texture
	 * @param samples Number of samples per pixel: 2, 4 or 8
	 */
	void create(const char* name, ivec2 size, PixelFormat format, GLsizei samples);

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
	void resize(ivec2 size);

	/**
	 * Bind the multisample texture to the specified texture unit. This allows
	 * it to be used in a shader for reading and/or writing.
	 * @param unit Texture unit to bind the multisample texture to
	 */
	void bind(TextureUnit unit);

};

/// OpenGL renderbuffer. Operates faster than a texture, but cannot be read
struct Renderbuffer : TextureBase {

	/**
	 * Create an OpenGL ID for the renderbuffer. This needs to be called before
	 * the renderbuffer can be used. Storage is allocated by default, and filled
	 * with garbage data.
	 * @param name Human-readable name, for logging and debug output
	 * @param size Initial size of the renderbuffer storage, in pixels
	 * @param format Internal format of the renderbuffer
	 */
	void create(const char* name, ivec2 size, PixelFormat format);

	/**
	 * Destroy the OpenGL renderbuffer object. Storage and ID are both freed.
	 */
	void destroy();

	/**
	 * Recreate the renderbuffer's storage with new size. Previous contents
	 * are lost, and the renderbuffer data is garbage again.
	 * @param size New size of the renderbuffer storage, in pixels
	 */
	void resize(ivec2 size);

};

/// OpenGL multisample renderbuffer. Operates faster than a multisample texture,
/// but cannot be read
struct RenderbufferMS : TextureBase {

	GLsizei samples = 0;

	/**
	 * Create an OpenGL ID for the multisample renderbuffer. This needs
	 * to be called before the renderbuffer can be used. Storage is allocated
	 * by default, and filled with garbage data.
	 * @param name Human-readable name, for logging and debug output
	 * @param size Initial size of the multisample renderbuffer storage,
	 * in pixels
	 * @param format Internal format of the multisample renderbuffer
	 * @param samples Number of samples per pixel: 2, 4 or 8
	 */
	void create(const char* name, ivec2 size, PixelFormat format, GLsizei samples);

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
	void resize(ivec2 size);

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

/// OpenGL framebuffer. Proxy object that allows drawing into textures
/// and renderbuffers from within shaders.
struct Framebuffer : GLObject {

	GLsizei samples = -1; ///< Sample count of all attachments needs to match
	bool dirty = true; ///< Is a glDrawBuffers call and completeness check needed?
	std::array<const TextureBase*, 17> attachments = {};

	/**
	 * Create an OpenGL ID for the framebuffer. This needs to be called before
	 * the framebuffer can be used. The object has no textures attached
	 * by default, and needs to have at least one color attachment attached
	 * to satisfy completeness requirements.
	 * @param name Human-readable name, for logging and debug output
	 */
	void create(const char* name);

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
	void attach(const Texture& t, Attachment attachment);

	/**
	 * Attach a multisample texture to a specified attachment point. All future
	 * attachments must have the same number of samples. The DepthStencil
	 * attachment can only hold a texture with a DepthStencil pixel format.
	 * Attachments cannot be overwritten.
	 * @param t Multisample texture to attach
	 * @param attachment Attachment point to attach to
	 */
	void attach(const TextureMS& t, Attachment attachment);

	/**
	 * Attach a renderbuffer to a specified attachment point. All future
	 * attachments must not be multisample. The DepthStencil attachment can only
	 * hold a renderbuffer with a DepthStencil pixel format. Attachments
	 * cannot be overwritten.
	 * @param t Renderbuffer to attach
	 * @param attachment Attachment point to attach to
	 */
	void attach(const Renderbuffer& r, Attachment attachment);

	/**
	 * Attach a multisample renderbuffer to a specified attachment point.
	 * All future attachments must have the same number of samples.
	 * The DepthStencil attachment can only hold a renderbuffer
	 * with a DepthStencil pixel format. Attachments cannot be overwritten.
	 * @param t Multisample renderbuffer to attach
	 * @param attachment Attachment point to attach to
	 */
	void attach(const RenderbufferMS& r, Attachment attachment);

	/**
	 * Bind this framebuffer to the OpenGL context, causing all future draw
	 * commands to render into the framebuffer's attachments. In a debug build,
	 * the framebuffer is checked for completeness.
	 */
	void bind();

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
	static void blit(Framebuffer& dst, const Framebuffer& src,
		Attachment srcBuffer = Attachment::Color0, bool depthStencil = false);

};

template<GLSLType T>
struct Uniform2 {

	using Type = T;

	GLint location = -1;
	Type value = {};

	void setLocation(Program program, const char* name);

	void set(Type val);

	inline auto operator=(Type val) -> Uniform2<Type>& { set(val); return *this; }

	inline auto operator=(const Uniform2& other) -> Uniform2& {
		if (&other == this)
			return *this;
		set(other);
	}

	inline operator Type&() { return value; }
	inline operator Type() const { return value; }

	Uniform2() = default;
	Uniform2(const Uniform2&) = delete;
	Uniform2(Uniform2&&) = delete;
	auto operator=(Uniform2&&) -> Uniform2& = delete;

};

/**
 * Base struct of ::Program type. To be a valid ::Program type usable with below
 * functions, a struct needs to have ::ProgramBase as its first element.
 */
typedef struct ProgramBase {
	Program id;
	const char* vertName; ///< Filename of the vertex shader for reference
	const char* fragName; ///< Filename of the fragment shader for reference
} ProgramBase;

/**
 * Create a new ::Program of specified valid type. Shaders are compiled, linked
 * and ready for use.
 * @param type A struct type with ::ProgramBase as its first element
 * @param vertName Name of the vertex shader
 * @param vertSrc GLSL source of the vertex shader
 * @param fragName Name of the fragment shader
 * @param fragSrc GLSL source of the fragment shader
 * @return Newly created Program. Must be destroyed with #programDestroy()
 */
#define programCreate(type, vertName, vertSrc, fragName, fragSrc) \
    ((type*)_programCreate(sizeof(type), (vertName), (vertSrc),   \
                                         (fragName), (fragSrc)))

/**
 * Destroy a ::Program. The pointer becomes invalid and cannot be used again.
 * @param program The ::Program object
 */
#define programDestroy(program) \
    (_programDestroy((ProgramBase*)(program)))

/**
 * Obtain a uniform location from the program. If it fails, returns -1 and logs
 * a warning.
 * @param program The ::Program object
 * @param uniform String literal of the uniform to query for
 * @return Uniform location in the program, or -1 on failure
 */
#define programUniform(program, uniform) \
    (_programUniform((ProgramBase*)(program), (uniform)))

/**
 * Set a sampler uniform to a specified texture unit. If it fails, logs
 * a warning.
 * @param program The Program object
 * @param sampler String literal of the sampler uniform to set
 * @param unit Texture unit to set @p sampler to
 * @return Value of @p unit
 */
#define programSampler(program, sampler, unit) \
    (_programSampler((ProgramBase*)(program), (sampler), (unit)))

/**
 * Activate a ::Program for rendering. The same ::Program stays active for any
 * number of draw calls until changed with another programUse().
 * @param program The Program object
 */
#define programUse(program) \
    (_programUse((ProgramBase*)(program)))

////////////////////////////////////////////////////////////////////////////////
// Implementation details

void* _programCreate(size_t size, const char* vertName, const char* vertSrc,
	const char* fragName, const char* fragSrc);

void _programDestroy(ProgramBase* program);

Uniform _programUniform(ProgramBase* program, const char* uniform);

TextureUnit
_programSampler(ProgramBase* program, const char* sampler, TextureUnit unit);

void _programUse(ProgramBase* program);

}

#include "sys/opengl.tpp"
