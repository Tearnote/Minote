/**
 * A thin OpenGL wrapper for dealing with most common objects
 * @file
 */

#pragma once

#include <cstddef>
#include "glad/glad.h"
#include "base/types.hpp"
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
	None = 0,
	Nearest = GL_NEAREST,
	Linear = GL_LINEAR
};

/// Available internal pixel formats
enum struct PixelFormat : GLint {
	None = 0,
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

/// Base struct for the common fields of texture types
struct TextureBase {

	GLuint id = 0; ///< The object has not been created if this is 0
	size2i size = {0, 0}; ///< The texture does not have storage if this is {0, 0}
	PixelFormat format = PixelFormat::None;

};

/// Standard 2D texture, usable for reading and writing inside shaders
struct Texture : TextureBase {

	Filter filter = Filter::None;

	/**
	 * Create an OpenGL ID for the texture. This needs to be called before
	 * the texture can be used. Storage is allocated by default, and filled
	 * with garbage data. The default filtering mode is Linear.
	 * @param size Initial size of the texture storage, in pixels
	 * @param format Internal format of the texture
	 */
	void create(size2i size, PixelFormat format);

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
	void resize(size2i size);

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
	 * @param size Initial size of the multisample texture storage, in pixels
	 * @param format Internal format of the multisample texture
	 * @param samples Number of samples per pixel: 2, 4 or 8
	 */
	void create(size2i size, PixelFormat format, GLsizei samples);

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
	void resize(size2i size);

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
	 * @param size Initial size of the renderbuffer storage, in pixels
	 * @param format Internal format of the renderbuffer
	 */
	void create(size2i size, PixelFormat format);

	/**
	 * Destroy the OpenGL renderbuffer object. Storage and ID are both freed.
	 */
	void destroy();

	/**
	 * Recreate the renderbuffer's storage with new size. Previous contents
	 * are lost, and the renderbuffer data is garbage again.
	 * @param size New size of the renderbuffer storage, in pixels
	 */
	void resize(size2i size);

};

/// OpenGL multisample renderbuffer. Operates faster than a multisample texture,
/// but cannot be read
struct RenderbufferMS : TextureBase {

	GLsizei samples = 0;

	/**
	 * Create an OpenGL ID for the multisample renderbuffer. This needs
	 * to be called before the renderbuffer can be used. Storage is allocated
	 * by default, and filled with garbage data.
	 * @param size Initial size of the multisample renderbuffer storage,
	 * in pixels
	 * @param format Internal format of the multisample renderbuffer
	 * @param samples Number of samples per pixel: 2, 4 or 8
	 */
	void create(size2i size, PixelFormat format, GLsizei samples);

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
	void resize(size2i size);

};

////////////////////////////////////////////////////////////////////////////////

/// OpenGL framebuffer. You can obtain an instance with framebufferCreate().
/// All fields read-only.
struct Framebuffer {

	GLuint id = 0;
	size2i size;
	GLsizei samples = 0;

};

/**
 * Create a new ::Framebuffer instance. After creation you may attach textures
 * and renderbuffers to it.
 * @return A newly created ::Framebuffer. Needs to be destroyed
 * with framebufferDestroy()
 */
Framebuffer* framebufferCreate(void);

/**
 * Destroy a ::Framebuffer instance. The destroyed object cannot be used
 * anymore and the pointer becomes invalid. All the textures and renderbuffers
 * bound to it are still intact, and need to be destroyed separately.
 * @param q The ::Framebuffer object
 */
void framebufferDestroy(Framebuffer* f);

/**
 * Attach a ::Texture to a specified attachment point. Framebuffer completeness
 * is not checked at this point, it needs to be done manually
 * with framebufferCheck() after all attachments are set up.
 * @param f The ::Framebuffer object
 * @param t ::Texture to attach
 * @param attachment Attachment point identifier. Equivalent to "attachment"
 * of glFramebufferTexture2D
 */
void framebufferTexture(Framebuffer* f, Texture& t, GLenum attachment);

/**
 * Attach a ::TextureMS to a specified attachment point. Framebuffer
 * completeness is not checked at this point, it needs to be done manually
 * with framebufferCheck() after all attachments are set up.
 * @param f The ::Framebuffer object
 * @param t ::TextureMS to attach
 * @param attachment Attachment point identifier. Equivalent to "attachment"
 * of glFramebufferTexture2D
 */
void framebufferTextureMS(Framebuffer* f, TextureMS& t, GLenum attachment);

/**
 * Attach a ::Renderbuffer to a specified attachment point. Framebuffer
 * completeness is not checked at this point, it needs to be done manually
 * with framebufferCheck() after all attachments are set up.
 * @param f The ::Framebuffer object
 * @param t ::Renderbuffer to attach
 * @param attachment Attachment point identifier. Equivalent to "attachment"
 * of glFramebufferRenderbuffer
 */
void
framebufferRenderbuffer(Framebuffer* f, Renderbuffer& r, GLenum attachment);

/**
 * Attach a ::RenderbufferMS to a specified attachment point. Framebuffer
 * completeness is not checked at this point, it needs to be done manually
 * with framebufferCheck() after all attachments are set up.
 * @param f The ::Framebuffer object
 * @param t ::RenderbufferMS to attach
 * @param attachment Attachment point identifier. Equivalent to "attachment"
 * of glFramebufferRenderbuffer
 */
void
framebufferRenderbufferMS(Framebuffer* f, RenderbufferMS& r, GLenum attachment);

/**
 * Set the ::Framebuffer's color outputs to the specified number of color
 * attachments. This should not be needed, but OpenGL isn't the best API
 * in the world. Only required for count of 2 or higher.
 * @param f The ::Framebuffer object
 * @param count Number of color framebuffers to use, starting
 * with GL_COLOR_ATTACHMENT0
 */
void framebufferBuffers(Framebuffer* f, GLsizei count);

/**
 * Check framebuffer completeness. To satisfy completeness, at least one color
 * attachment needs to be set, all attached objects need to be valid,
 * and they all need to have the same sample count.
 * @param f The ::Framebuffer object
 * @return true if complete, false if incomplete
 */
bool framebufferCheck(Framebuffer* f);

/**
 * Bind a ::Framebuffer, so that all future draw calls write into the attached
 * objects instead of the screen.
 * @param f The ::Framebuffer object, or #null for the backbuffer
 */
void framebufferUse(Framebuffer* f);

/**
 * Copy a contents of a ::Framebuffer to the screen (backbuffer).
 * @param f The ::Framebuffer object
 */
void framebufferToScreen(Framebuffer* f, Window& w);

/**
 * Copy the contents of one ::Framebuffer to another. Performs MSAA resolve.
 * @param src The source ::Framebuffer object
 * @param dst The destination ::Framebuffer object
 * @param size Size of the area to copy, in pixels
 */
void framebufferBlit(Framebuffer* src, Framebuffer* dst, size2i size);

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
