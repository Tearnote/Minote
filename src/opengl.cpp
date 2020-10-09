/**
 * Implementation of opengl.hpp
 * @file
 */

#include "opengl.hpp"

#include "base/log.hpp"

namespace minote {

/// OpenGL shader object ID
using Shader = GLuint;

GLuint boundFb = 0;
GLuint boundProgram = 0;

TextureMS* textureMSCreate(void)
{
	auto* t = allocate<TextureMS>();
	glGenTextures(1, &t->id);
	ASSERT(t->id);
	return t;
}

void textureMSDestroy(TextureMS* t)
{
	if (!t) return;
	glDeleteTextures(1, &t->id);
	free(t);
}

void textureMSStorage(TextureMS* t, size2i size, GLenum format, GLsizei samples)
{
	ASSERT(t);
	ASSERT(size.x > 0);
	ASSERT(size.y > 0);
	ASSERT(samples >= 2);
	glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, t->id);
	glTexImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, samples, format,
		size.x, size.y, GL_TRUE);
	t->size.x = size.x;
	t->size.y = size.y;
	t->samples = samples;
}

void textureMSUse(TextureMS* t, TextureUnit unit)
{
	ASSERT(t);
	glActiveTexture(unit);
	glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, t->id);
}

Renderbuffer* renderbufferCreate(void)
{
	auto* r = allocate<Renderbuffer>();
	glGenRenderbuffers(1, &r->id);
	return r;
}

void renderbufferDestroy(Renderbuffer* r)
{
	if (!r) return;
	glDeleteRenderbuffers(1, &r->id);
	free(r);
}

void renderbufferStorage(Renderbuffer* r, size2i size, GLenum format)
{
	ASSERT(r);
	ASSERT(size.x > 0);
	ASSERT(size.y > 0);
	glBindRenderbuffer(GL_RENDERBUFFER, r->id);
	glRenderbufferStorage(GL_RENDERBUFFER, format, size.x, size.y);
}

RenderbufferMS* renderbufferMSCreate(void)
{
	auto* r = allocate<RenderbufferMS>();
	glGenRenderbuffers(1, &r->id);
	return r;
}

void renderbufferMSDestroy(RenderbufferMS* r)
{
	if (!r) return;
	glDeleteRenderbuffers(1, &r->id);
	free(r);
}

void renderbufferMSStorage(RenderbufferMS* r, size2i size, GLenum format,
	GLsizei samples)
{
	ASSERT(r);
	ASSERT(size.x > 0);
	ASSERT(size.y > 0);
	ASSERT(samples >= 2);
	glBindRenderbuffer(GL_RENDERBUFFER, r->id);
	glRenderbufferStorageMultisample(GL_RENDERBUFFER, samples, format,
		size.x, size.y);
	r->size.x = size.x;
	r->size.y = size.y;
	r->samples = samples;
}

Framebuffer* framebufferCreate(void)
{
	auto* f = allocate<Framebuffer>();
	glGenFramebuffers(1, &f->id);
	ASSERT(f->id);
	return f;
}

void framebufferDestroy(Framebuffer* f)
{
	if (!f) return;
	glDeleteFramebuffers(1, &f->id);
	free(f);
}

void framebufferTexture(Framebuffer* f, Texture& t, GLenum attachment)
{
	ASSERT(f);
	framebufferUse(f);
	glFramebufferTexture2D(GL_FRAMEBUFFER, attachment, GL_TEXTURE_2D, t.id, 0);
}

void framebufferTextureMS(Framebuffer* f, TextureMS* t, GLenum attachment)
{
	ASSERT(f);
	ASSERT(t);
	framebufferUse(f);
	glFramebufferTexture2D(GL_FRAMEBUFFER, attachment,
		GL_TEXTURE_2D_MULTISAMPLE, t->id, 0);
}

void framebufferRenderbuffer(Framebuffer* f, Renderbuffer* r, GLenum attachment)
{
	ASSERT(f);
	ASSERT(r);
	framebufferUse(f);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, attachment, GL_RENDERBUFFER,
		r->id);
}

void
framebufferRenderbufferMS(Framebuffer* f, RenderbufferMS* r, GLenum attachment)
{
	ASSERT(f);
	ASSERT(r);
	framebufferUse(f);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, attachment, GL_RENDERBUFFER,
		r->id);
}

void framebufferBuffers(Framebuffer* f, GLsizei count)
{
	ASSERT(f);
	ASSERT(count >= 1 && count <= 16);
	GLenum attachments[count];
	for (size_t i = 0; i <= count; i += 1)
		attachments[i] = GL_COLOR_ATTACHMENT0 + i;

	framebufferUse(f);
	glDrawBuffers(count, attachments);
}

bool framebufferCheck(Framebuffer* f)
{
	ASSERT(f);
	framebufferUse(f);
	return glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE;
}

void framebufferUse(Framebuffer* f)
{
	GLuint target = 0;
	if (f) target = f->id;
	if (boundFb == target) return;
	glBindFramebuffer(GL_FRAMEBUFFER, target);
	boundFb = target;
}

void framebufferToScreen(Framebuffer* f, Window& w)
{
	ASSERT(f);
	framebufferUse(f);
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
	boundFb = 0;

	size2i screenSize = w.size;
	glBlitFramebuffer(0, 0, screenSize.x, screenSize.y,
		0, 0, screenSize.x, screenSize.y,
		GL_COLOR_BUFFER_BIT, GL_NEAREST);
}

void framebufferBlit(Framebuffer* src, Framebuffer* dst, size2i size)
{
	ASSERT(src);
	ASSERT(dst);
	ASSERT(size.x > 0);
	ASSERT(size.y > 0);
	framebufferUse(dst);
	glBindFramebuffer(GL_READ_FRAMEBUFFER, src->id);
	glBlitFramebuffer(0, 0, size.x, size.y,
		0, 0, size.x, size.y,
		GL_COLOR_BUFFER_BIT, GL_NEAREST);
}

/**
 * Create an OpenGL shader object. The shader is compiled and ready for linking.
 * @param name Human-readable name for reference
 * @param source Struct containing shader's name and GLSL source code
 * @param type GL_VERTEX_SHADER or GL_FRAGMENT_SHADER
 * @return Newly created ::Shader's ID
 */
static Shader shaderCreate(const char* name, const char* source, GLenum type)
{
	ASSERT(name);
	ASSERT(source);
	ASSERT(type == GL_VERTEX_SHADER || type == GL_FRAGMENT_SHADER);
	Shader shader = glCreateShader(type);
	glShaderSource(shader, 1, &source, nullptr);
	glCompileShader(shader);
	GLint compileStatus = 0;
	glGetShaderiv(shader, GL_COMPILE_STATUS, &compileStatus);
	if (compileStatus == GL_FALSE) {
		GLchar infoLog[512];
		glGetShaderInfoLog(shader, 512, nullptr, infoLog);
		L.error("Failed to compile shader %s: %s", name, infoLog);
		glDeleteShader(shader);
		return 0;
	}
	ASSERT(shader);
	L.debug("Compiled %s shader %s",
		type == GL_VERTEX_SHADER ? "vertex" : "fragment", name);
	return shader;
}

/**
 * Destroy a ::Shader instance. The shader ID becomes invalid and cannot be
 * used again.
 * @param shader ::Shader ID to destroy
 */
static void shaderDestroy(Shader shader)
{
	glDeleteShader(shader);
}

void* _programCreate(size_t size, const char* vertName, const char* vertSrc,
	const char* fragName, const char* fragSrc)
{
	ASSERT(size);
	ASSERT(vertName);
	ASSERT(vertSrc);
	ASSERT(fragName);
	ASSERT(fragSrc);
	// Overallocating memory so that it fits in the user's provided struct type
	auto* result = reinterpret_cast<ProgramBase*>(allocate<uint8_t>(size));
	result->vertName = vertName;
	result->fragName = fragName;
	Shader vert = shaderCreate(vertName, vertSrc, GL_VERTEX_SHADER);
	if (vert == 0)
		return result;
	Shader frag = shaderCreate(fragName, fragSrc, GL_FRAGMENT_SHADER);
	if (frag == 0) {
		shaderDestroy(vert); // Proper cleanup, how fancy
		return result;
	}

	result->id = glCreateProgram();
	glAttachShader(result->id, vert);
	glAttachShader(result->id, frag);
	glLinkProgram(result->id);
	GLint linkStatus = 0;
	glGetProgramiv(result->id, GL_LINK_STATUS, &linkStatus);
	if (linkStatus == GL_FALSE) {
		GLchar infoLog[512];
		glGetProgramInfoLog(result->id, 512, nullptr, infoLog);
		L.error("Failed to link shader program %s+%s: %s",
			vertName, fragName, infoLog);
		glDeleteProgram(result->id);
		result->id = 0;
	}
	shaderDestroy(frag);
	frag = 0;
	shaderDestroy(vert);
	vert = 0;
	L.debug("Linked shader program %s+%s", vertName, fragName);
	return result;
}

void _programDestroy(ProgramBase* program)
{
	if (!program) return;
	glDeleteProgram(program->id);
	program->id = 0;
	L.debug("Destroyed shader program %s+%s",
		program->vertName, program->fragName);
	free(program);
	program = nullptr;
}

Uniform _programUniform(ProgramBase* program, const char* uniform)
{
	ASSERT(program);
	ASSERT(uniform);
	Uniform result = glGetUniformLocation(program->id, uniform);
	if (result == -1)
		L.warn("\"%s\" uniform not available in shader program %s+%s",
			uniform, program->vertName, program->fragName);
	return result;
}

TextureUnit
_programSampler(ProgramBase* program, const char* sampler, TextureUnit unit)
{
	ASSERT(program);
	ASSERT(sampler);
	Uniform uniform = programUniform(program, sampler);
	if (uniform != -1) {
		programUse(program);
		glUniform1i(uniform, unit - GL_TEXTURE0);
	}
	return unit;
}

void _programUse(ProgramBase* program)
{
	ASSERT(program);
	if (program->id == boundProgram) return;
	glUseProgram(program->id);
	boundProgram = program->id;
}

////////////////////////////////////////////////////////////////////////////////

/**
 * Convert the strictly typed PixelFormat enum into the equivalent
 * OpenGL internalformat value.
 * @param format PixelFormat to convert
 * @return Matching internalformat GLenum value
 */
constexpr static auto pixelFormatToGLInternal(const PixelFormat format) -> GLenum
{
	switch (format) {
	case PixelFormat::R_u8:
		return GL_R8;
	case PixelFormat::RG_u8:
		return GL_RG8;
	case PixelFormat::RGB_u8:
		return GL_RGB8;
	case PixelFormat::RGBA_u8:
		return GL_RGBA8;
	case PixelFormat::R_f16:
		return GL_R16F;
	case PixelFormat::RG_f16:
		return GL_RG16F;
	case PixelFormat::RGB_f16:
		return GL_RGB16F;
	case PixelFormat::RGBA_f16:
		return GL_RGBA16F;
	default:
		ASSERT(false, "Invalid PixelFormat %d", +format);
		return 0;
	}
}

/**
 * Convert the strictly typed PixelFormat enum into the equivalent
 * OpenGL format value.
 * @param format PixelFormat to convert
 * @return Matching format GLenum value
 */
constexpr static auto pixelFormatToGLExternal(const PixelFormat format) -> GLenum
{
	switch (format) {
	case PixelFormat::R_u8:
	case PixelFormat::R_f16:
		return GL_RED;
	case PixelFormat::RG_u8:
	case PixelFormat::RG_f16:
		return GL_RG;
	case PixelFormat::RGB_u8:
	case PixelFormat::RGB_f16:
		return GL_RGB;
	case PixelFormat::RGBA_u8:
	case PixelFormat::RGBA_f16:
		return GL_RGBA;
	default:
		ASSERT(false, "Invalid PixelFormat %d", +format);
		return 0;
	}
}

constexpr static auto filterToGL(const Filter filter) -> GLint
{
	switch (filter) {
	case Filter::Nearest:
		return GL_NEAREST;
	case Filter::Linear:
		return GL_LINEAR;
	default:
		ASSERT(false, "Invalid Filter %d", +filter);
		return 0;
	}
}

void Texture::create(size2i _size, PixelFormat _format)
{
	ASSERT(!id);
	ASSERT(_format != PixelFormat::None);

	glGenTextures(1, &id);
	glBindTexture(GL_TEXTURE_2D, id);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	setFilter(Filter::Linear);
	format = _format;
	resize(_size);
}

void Texture::destroy()
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
	format = PixelFormat::None;
}

void Texture::setFilter(Filter _filter)
{
	ASSERT(_filter != Filter::None);
	if (filter == _filter)
		return;

	const GLint newFilter = filterToGL(_filter);
	glBindTexture(GL_TEXTURE_2D, id);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, newFilter);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, newFilter);
	filter = _filter;
}

void Texture::resize(size2i _size)
{
	ASSERT(_size.x > 0 && _size.y > 0);
	ASSERT(id);
	if (size == _size)
		return;

	glBindTexture(GL_TEXTURE_2D, id);
	glTexImage2D(GL_TEXTURE_2D, 0, pixelFormatToGLInternal(format),
		_size.x, _size.y, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
	size = _size;
}

void Texture::upload(std::uint8_t* data)
{
	ASSERT(data);
	ASSERT(id);
	ASSERT(size.x > 0 && size.y > 0);

	glBindTexture(GL_TEXTURE_2D, id);
	glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, size.x, size.y,
		pixelFormatToGLExternal(format), GL_UNSIGNED_BYTE, data);
}

void Texture::bind(TextureUnit unit)
{
	ASSERT(id);

	glActiveTexture(unit);
	glBindTexture(GL_TEXTURE_2D, id);
}

}
