/**
 * A thin OpenGL wrapper for dealing with most common objects
 * @file
 */

#ifndef MINOTE_OPENGL_H
#define MINOTE_OPENGL_H

#include <stdbool.h>
#include <stddef.h>
#include "glad/glad.h"
#include "base/types.hpp"
#include "sys/window.hpp"

/// OpenGL texture. You can obtain an instance with textureCreate().
/// All fields read-only.
typedef struct Texture {
	GLuint id;
	minote::size2i size;
} Texture;

/// OpenGL multisample texture. You can obtain an instance with textureMSCreate().
/// All fields read-only.
typedef struct TextureMS {
	GLuint id;
	minote::size2i size;
	GLsizei samples;
} TextureMS;

/// OpenGL renderbuffer. You can obtain an instance with renderbufferCreate().
/// All fields read-only.
typedef struct Renderbuffer {
	GLuint id;
	minote::size2i size;
} Renderbuffer;

/// OpenGL multisample renderbuffer. You can obtain an instance with renderbufferMSCreate().
/// All fields read-only.
typedef struct RenderbufferMS {
	GLuint id;
	minote::size2i size;
	GLsizei samples;
} RenderbufferMS;

/// OpenGL framebuffer. You can obtain an instance with framebufferCreate().
/// All fields read-only.
typedef struct Framebuffer {
	GLuint id;
	minote::size2i size;
	GLsizei samples;
} Framebuffer;

/// OpenGL shader program. You can obtain an instance with programCreate().
typedef GLuint Program;

/// OpenGL uniform location. This is used as part of a struct containing ::ProgramBase.
typedef GLint Uniform;

/// OpenGL texture unit. This is used as part of a struct containing ::ProgramBase.
typedef GLenum TextureUnit;

/// OpenGL vertex buffer object ID
typedef GLuint VertexBuffer;

/// OpenGL vertex array object ID
typedef GLuint VertexArray;

/// OpenGL element array object ID
typedef GLuint ElementArray;

/// OpenGL buffer texture object ID
typedef GLuint BufferTexture;

/// OpenGL buffer object, for use with buffer textures
typedef GLuint BufferTextureStorage;

/**
 * Create a new ::Texture instance. Please note that this object cannot be used
 * for drawing or sampling until storage is allocated with textureStorage().
 * @return A newly created ::Texture. Needs to be destroyed with textureDestroy()
 */
Texture* textureCreate(void);

/**
 * Destroy a ::Texture instance. The destroyed object cannot be used anymore and
 * the pointer becomes invalid.
 * @param q The ::Texture object
 */
void textureDestroy(Texture* t);

/**
 * Set a ::Texture's filtering mode. The default value is GL_LINEAR.
 * @param t The ::Texture object
 * @param filteringMode GL_LINEAR or GL_NEAREST
 */
void textureFilter(Texture* t, GLenum filteringMode);

/**
 * Allocate storage to a ::Texture. After this call, it can be used
 * for rendering and sampling. Contents are undefined until drawn into
 * or specified with textureData(). Can be called more than once to orphan
 * old storage and create a brand new surface with different parameters.
 * @param t The ::Texture object
 * @param size Size of the allocated buffer, in pixels
 * @param format Internal storage format. Equivalent to "internalformat" of
 * glTexImage2D
 */
void textureStorage(Texture* t, minote::size2i size, GLenum format);

/**
 * Upload data to a ::Texture. Storage must have been allocated first
 * with textureStorage(). No sanity checks are performed - @a data must be
 * as big as number of pixels in allocated storage times size of each pixel
 * according to @a format and @a type.
 * @param t The ::Texture object
 * @param data Pointer to raw data buffer to upload
 * @param format Format of pixel @a data being uploaded. Equivalent to "format"
 * of glTexImage2D
 * @param type Type of each value in @a data. Equivalent to "type"
 * of glTexImage2D
 */
void textureData(Texture* t, void* data, GLenum format, GLenum type);

/**
 * Bind a ::Texture to a specified ::TextureUnit, allowing it to be sampled
 * by a ::Program.
 * @param t The ::Texture object
 * @param unit Unit number obtained from programSampler()
 */
void textureUse(Texture* t, TextureUnit unit);

/**
 * Create a new ::TextureMS instance. Please note that this object cannot
 * be used for drawing into until storage is allocated with textureStorage().
 * @return A newly created ::TextureMS. Needs to be destroyed with textureMSDestroy()
 */
TextureMS* textureMSCreate(void);

/**
 * Destroy a ::TextureMS instance. The destroyed object cannot be used anymore
 * and the pointer becomes invalid.
 * @param q The ::TextureMS object
 */
void textureMSDestroy(TextureMS* t);

/**
 * Allocate storage to a ::TextureMS. After this call, it can be used
 * for rendering into. Contents are undefined until drawn into. Can be called
 * more than once to orphan old storage and create a brand new surface
 * with different parameters.
 * @param t The ::TextureMS object
 * @param size Size of the allocated buffer, in pixels
 * @param format Internal storage format. Equivalent to "internalformat" of
 * glTexImage2DMultisample
 * @param samples Number of samples per pixel
 */
void textureMSStorage(TextureMS* t, minote::size2i size, GLenum format, GLsizei samples);

/**
 * Bind a ::TextureMS to a specified ::TextureUnit, allowing it to be sampled
 * by a ::Program.
 * @param t The ::TextureMS object
 * @param unit Unit number obtained from programSampler()
 */
void textureMSUse(TextureMS* t, TextureUnit unit);

/**
 * Create a new ::Renderbuffer instance. Please note that this object cannot
 * be used for drawing into until storage is allocated with renderbufferStorage().
 * @return A newly created ::Renderbuffer. Needs to be destroyed
 * with renderbufferDestroy()
 */
Renderbuffer* renderbufferCreate(void);

/**
 * Destroy a ::Renderbuffer instance. The destroyed object cannot be used
 * anymore and the pointer becomes invalid.
 * @param q The ::Renderbuffer object
 */
void renderbufferDestroy(Renderbuffer* r);

/**
 * Allocate storage to a ::Renderbuffer. After this call, it can be used
 * for rendering into. Contents are undefined until drawn into. Can be called
 * more than once to orphan old storage and create a brand new surface
 * with different parameters.
 * @param t The ::Renderbuffer object
 * @param size Size of the allocated buffer, in pixels
 * @param format Internal storage format. Equivalent to "internalformat" of
 * glRenderbufferStorage
 */
void renderbufferStorage(Renderbuffer* r, minote::size2i size, GLenum format);

/**
 * Create a new ::RenderbufferMS instance. Please note that this object cannot
 * be used for drawing into until storage is allocated with renderbufferMSStorage().
 * @return A newly created ::RenderbufferMS. Needs to be destroyed
 * with renderbufferMSDestroy()
 */
RenderbufferMS* renderbufferMSCreate(void);

/**
 * Destroy a ::RenderbufferMS instance. The destroyed object cannot be used
 * anymore and the pointer becomes invalid.
 * @param q The ::RenderbufferMS object
 */
void renderbufferMSDestroy(RenderbufferMS* r);

/**
 * Allocate storage to a ::RenderbufferMS. After this call, it can be used
 * for rendering into. Contents are undefined until drawn into. Can be called
 * more than once to orphan old storage and create a brand new surface
 * with different parameters.
 * @param t The ::RenderbufferMS object
 * @param size Size of the allocated buffer, in pixels
 * @param format Internal storage format. Equivalent to "internalformat" of
 * glRenderbufferStorageMultisample
 * @param samples Number of samples per pixel
 */
void renderbufferMSStorage(RenderbufferMS* r, minote::size2i size, GLenum format,
	GLsizei samples);

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
void framebufferTexture(Framebuffer* f, Texture* t, GLenum attachment);

/**
 * Attach a ::TextureMS to a specified attachment point. Framebuffer
 * completeness is not checked at this point, it needs to be done manually
 * with framebufferCheck() after all attachments are set up.
 * @param f The ::Framebuffer object
 * @param t ::TextureMS to attach
 * @param attachment Attachment point identifier. Equivalent to "attachment"
 * of glFramebufferTexture2D
 */
void framebufferTextureMS(Framebuffer* f, TextureMS* t, GLenum attachment);

/**
 * Attach a ::Renderbuffer to a specified attachment point. Framebuffer
 * completeness is not checked at this point, it needs to be done manually
 * with framebufferCheck() after all attachments are set up.
 * @param f The ::Framebuffer object
 * @param t ::Renderbuffer to attach
 * @param attachment Attachment point identifier. Equivalent to "attachment"
 * of glFramebufferRenderbuffer
 */
void framebufferRenderbuffer(Framebuffer* f, Renderbuffer* r, GLenum attachment);

/**
 * Attach a ::RenderbufferMS to a specified attachment point. Framebuffer
 * completeness is not checked at this point, it needs to be done manually
 * with framebufferCheck() after all attachments are set up.
 * @param f The ::Framebuffer object
 * @param t ::RenderbufferMS to attach
 * @param attachment Attachment point identifier. Equivalent to "attachment"
 * of glFramebufferRenderbuffer
 */
void framebufferRenderbufferMS(Framebuffer* f, RenderbufferMS* r, GLenum attachment);

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
void framebufferToScreen(Framebuffer* f, minote::Window& w);

/**
 * Copy the contents of one ::Framebuffer to another. Performs MSAA resolve.
 * @param src The source ::Framebuffer object
 * @param dst The destination ::Framebuffer object
 * @param size Size of the area to copy, in pixels
 */
void framebufferBlit(Framebuffer* src, Framebuffer* dst, minote::size2i size);

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

#endif //MINOTE_OPENGL_H
