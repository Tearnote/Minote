/**
 * Implementation of opengl.hpp
 * @file
 */

#include "sys/opengl.hpp"

#include "base/log.hpp"

namespace minote {

/// OpenGL shader object ID
using Shader = GLuint;

GLuint boundFb = 0;
GLuint boundProgram = 0;

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

void framebufferTextureMS(Framebuffer* f, TextureMS& t, GLenum attachment)
{
		ASSERT(f);
	framebufferUse(f);
	glFramebufferTexture2D(GL_FRAMEBUFFER, attachment,
		GL_TEXTURE_2D_MULTISAMPLE, t.id, 0);
}

void framebufferRenderbuffer(Framebuffer* f, Renderbuffer& r, GLenum attachment)
{
		ASSERT(f);
	framebufferUse(f);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, attachment, GL_RENDERBUFFER,
		r.id);
}

void
framebufferRenderbufferMS(Framebuffer* f, RenderbufferMS& r, GLenum attachment)
{
		ASSERT(f);
	framebufferUse(f);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, attachment, GL_RENDERBUFFER,
		r.id);
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

	glBindTexture(GL_TEXTURE_2D, id);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, +_filter);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, +_filter);
	filter = _filter;
}

void Texture::resize(size2i _size)
{
	ASSERT(_size.x > 0 && _size.y > 0);
	ASSERT(id);
	if (size == _size)
		return;

	glBindTexture(GL_TEXTURE_2D, id);
	glTexImage2D(GL_TEXTURE_2D, 0, +format,
		_size.x, _size.y, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
	size = _size;
}

void Texture::upload(std::uint8_t* data)
{
	ASSERT(data);
	ASSERT(id);
	ASSERT(size.x > 0 && size.y > 0);
	ASSERT(format != PixelFormat::DepthStencil);

	const GLenum channels = [=] {
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
		case PixelFormat::DepthStencil:
		default:
			ASSERT(false, "Invalid PixelFormat %d", +format);
			return 0;
		}
	}();

	glBindTexture(GL_TEXTURE_2D, id);
	glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, size.x, size.y,
		channels, GL_UNSIGNED_BYTE, data);
}

void Texture::bind(TextureUnit unit)
{
	ASSERT(id);

	glActiveTexture(unit);
	glBindTexture(GL_TEXTURE_2D, id);
}

void TextureMS::create(size2i _size, PixelFormat _format, GLsizei _samples)
{
	ASSERT(!id);
	ASSERT(_format != PixelFormat::None);
	ASSERT(_samples == 2 || _samples == 4 || _samples == 8);

	glGenTextures(1, &id);
	format = _format;
	samples = _samples;
	resize(_size);
}

void TextureMS::destroy()
{
#ifndef NDEBUG
	if (!id) {
		L.warn("Tried to destroy a multisample texture that has not been created");
		return;
	}
#endif //NDEBUG

	glDeleteTextures(1, &id);
	id = 0;
	size = {0, 0};
	format = PixelFormat::None;
	samples = 0;
}

void TextureMS::resize(size2i _size)
{
	ASSERT(_size.x > 0 && _size.y > 0);
	ASSERT(id);
	if (size == _size)
		return;

	glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, id);
	glTexImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, samples, +format,
		_size.x, _size.y, GL_TRUE);
	size = _size;
}

void TextureMS::bind(TextureUnit unit)
{
	ASSERT(id);

	glActiveTexture(unit);
	glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, id);
}

void Renderbuffer::create(size2i _size, PixelFormat _format)
{
	ASSERT(!id);
	ASSERT(_format != PixelFormat::None);

	glGenRenderbuffers(1, &id);
	format = _format;
	resize(_size);
}

void Renderbuffer::destroy()
{
#ifndef NDEBUG
	if (!id) {
		L.warn("Tried to destroy a renderbuffer that has not been created");
		return;
	}
#endif //NDEBUG

	glDeleteRenderbuffers(1, &id);
	id = 0;
	size = {0, 0};
	format = PixelFormat::None;
}

void Renderbuffer::resize(size2i _size)
{
	ASSERT(_size.x > 0 && _size.y > 0);
	ASSERT(id);
	if (size == _size)
		return;

	glBindRenderbuffer(GL_RENDERBUFFER, id);
	glRenderbufferStorage(GL_RENDERBUFFER, +format, _size.x, _size.y);
	size = _size;
}

void RenderbufferMS::create(size2i _size, PixelFormat _format, GLsizei _samples)
{
	ASSERT(!id);
	ASSERT(_format != PixelFormat::None);
	ASSERT(_samples == 2 || _samples == 4 || _samples == 8);

	glGenRenderbuffers(1, &id);
	format = _format;
	samples = _samples;
	resize(_size);
}

void RenderbufferMS::destroy()
{
#ifndef NDEBUG
	if (!id) {
		L.warn("Tried to destroy a multisample renderbuffer that has not been created");
		return;
	}
#endif //NDEBUG

	glDeleteRenderbuffers(1, &id);
	id = 0;
	size = {0, 0};
	format = PixelFormat::None;
}

void RenderbufferMS::resize(size2i _size)
{
	ASSERT(_size.x > 0 && _size.y > 0);
	ASSERT(id);
	if (size == _size)
		return;

	glBindRenderbuffer(GL_RENDERBUFFER, id);
	glRenderbufferStorageMultisample(GL_RENDERBUFFER, samples,+format,
		_size.x, _size.y);
	size = _size;
}

}
