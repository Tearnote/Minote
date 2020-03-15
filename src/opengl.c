/**
 * Implementation of opengl.h
 * @file
 */

#include "opengl.h"

#include <assert.h>
#include "util.h"
#include "log.h"

struct Texture {
	GLuint id;
	size2i size;
};

struct TextureMS {
	GLuint id;
	size2i size;
	GLsizei samples;
};

struct Renderbuffer {
	GLuint id;
	size2i size;
};

struct RenderbufferMS {
	GLuint id;
	size2i size;
	GLsizei samples;
};

struct Framebuffer {
	GLuint id;
	size2i size;
	GLsizei samples;
};

/// OpenGL shader object ID
typedef GLuint Shader;

GLuint boundFb = 0;
GLuint boundProgram = 0;

Texture* textureCreate(void)
{
	Texture* t = alloc(sizeof(*t));
	glGenTextures(1, &t->id);
	assert(t->id);
	glBindTexture(GL_TEXTURE_2D, t->id);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	return t;
}

void textureDestroy(Texture* t)
{
	if (!t) return;
	glDeleteTextures(1, &t->id);
	free(t);
}

void textureFilter(Texture* t, GLenum filteringMode)
{
	assert(t);
	assert(filteringMode == GL_NEAREST || filteringMode == GL_LINEAR);
	glBindTexture(GL_TEXTURE_2D, t->id);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, filteringMode);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, filteringMode);
}

void textureStorage(Texture* t, size2i size, GLenum format)
{
	assert(t);
	assert(size.x > 0);
	assert(size.y > 0);
	glBindTexture(GL_TEXTURE_2D, t->id);
	glTexImage2D(GL_TEXTURE_2D, 0, format, size.x, size.y,
		0, GL_RGBA, GL_UNSIGNED_BYTE, null);
	t->size.x = size.x;
	t->size.y = size.y;
}

void textureData(Texture* t, void* data, GLenum format, GLenum type)
{
	assert(t);
	assert(t->size.x > 0);
	assert(t->size.y > 0);
	assert(data);
	glBindTexture(GL_TEXTURE_2D, t->id);
	glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, t->size.x, t->size.y,
		format, type, data);
}

void textureUse(Texture* t, TextureUnit unit)
{
	assert(t);
	glActiveTexture(unit);
	glBindTexture(GL_TEXTURE_2D, t->id);
}

TextureMS* textureMSCreate(void)
{
	TextureMS* t = alloc(sizeof(*t));
	glGenTextures(1, &t->id);
	assert(t->id);
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
	assert(t);
	assert(size.x > 0);
	assert(size.y > 0);
	assert(samples >= 2);
	glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, t->id);
	glTexImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, samples, format,
		size.x, size.y, GL_TRUE);
	t->size.x = size.x;
	t->size.y = size.y;
	t->samples = samples;
}

void textureMSUse(TextureMS* t, TextureUnit unit)
{
	assert(t);
	glActiveTexture(unit);
	glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, t->id);
}

Renderbuffer* renderbufferCreate(void)
{
	Renderbuffer* r = alloc(sizeof(*r));
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
	assert(r);
	assert(size.x > 0);
	assert(size.y > 0);
	glBindRenderbuffer(GL_RENDERBUFFER, r->id);
	glRenderbufferStorage(GL_RENDERBUFFER, format, size.x, size.y);
}

RenderbufferMS* renderbufferMSCreate(void)
{
	RenderbufferMS* r = alloc(sizeof(*r));
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
	assert(r);
	assert(size.x > 0);
	assert(size.y > 0);
	assert(samples >= 2);
	glBindRenderbuffer(GL_RENDERBUFFER, r->id);
	glRenderbufferStorageMultisample(GL_RENDERBUFFER, samples, format,
		size.x, size.y);
	r->size.x = size.x;
	r->size.y = size.y;
	r->samples = samples;
}

Framebuffer* framebufferCreate(void)
{
	Framebuffer* f = alloc(sizeof(*f));
	glGenFramebuffers(1, &f->id);
	assert(f->id);
	return f;
}

void framebufferDestroy(Framebuffer* f)
{
	if (!f) return;
	glDeleteFramebuffers(1, &f->id);
	free(f);
}

void framebufferTexture(Framebuffer* f, Texture* t, GLenum attachment)
{
	assert(f);
	assert(t);
	framebufferUse(f);
	glFramebufferTexture2D(GL_FRAMEBUFFER, attachment, GL_TEXTURE_2D, t->id, 0);
}

void framebufferTextureMS(Framebuffer* f, TextureMS* t, GLenum attachment)
{
	assert(f);
	assert(t);
	framebufferUse(f);
	glFramebufferTexture2D(GL_FRAMEBUFFER, attachment,
		GL_TEXTURE_2D_MULTISAMPLE, t->id, 0);
}

void framebufferRenderbuffer(Framebuffer* f, Renderbuffer* r, GLenum attachment)
{
	assert(f);
	assert(r);
	framebufferUse(f);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, attachment, GL_RENDERBUFFER,
		r->id);
}

void
framebufferRenderbufferMS(Framebuffer* f, RenderbufferMS* r, GLenum attachment)
{
	assert(f);
	assert(r);
	framebufferUse(f);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, attachment, GL_RENDERBUFFER,
		r->id);
}

void framebufferBuffers(Framebuffer* f, GLsizei count)
{
	assert(f);
	assert(count >= 1 && count <= 16);
	GLenum attachments[count];
	for (size_t i = 0; i <= count; i += 1)
		attachments[i] = GL_COLOR_ATTACHMENT0 + i;

	framebufferUse(f);
	glDrawBuffers(count, attachments);
}

bool framebufferCheck(Framebuffer* f)
{
	assert(f);
	framebufferUse(f);
	return glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE;
}

void framebufferUse(Framebuffer* f)
{
	assert(f);
	if (boundFb == f->id) return;
	glBindFramebuffer(GL_FRAMEBUFFER, f->id);
	boundFb = f->id;
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
	assert(name);
	assert(source);
	assert(type == GL_VERTEX_SHADER || type == GL_FRAGMENT_SHADER);
	Shader shader = glCreateShader(type);
	glShaderSource(shader, 1, &source, null);
	glCompileShader(shader);
	GLint compileStatus = 0;
	glGetShaderiv(shader, GL_COMPILE_STATUS, &compileStatus);
	if (compileStatus == GL_FALSE) {
		GLchar infoLog[512];
		glGetShaderInfoLog(shader, 512, null, infoLog);
		logError(applog, u8"Failed to compile shader %s: %s", name, infoLog);
		glDeleteShader(shader);
		return 0;
	}
	assert(shader);
	logDebug(applog, u8"Compiled %s shader %s",
		type == GL_VERTEX_SHADER ? u8"vertex" : u8"fragment", name);
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
	assert(size);
	assert(vertName);
	assert(vertSrc);
	assert(fragName);
	assert(fragSrc);
	// Overallocating memory so that it fits in the user's provided struct type
	ProgramBase* result = alloc(size);
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
		glGetProgramInfoLog(result->id, 512, null, infoLog);
		logError(applog, u8"Failed to link shader program %s+%s: %s",
			vertName, fragName, infoLog);
		glDeleteProgram(result->id);
		result->id = 0;
	}
	shaderDestroy(frag);
	frag = 0;
	shaderDestroy(vert);
	vert = 0;
	logDebug(applog, u8"Linked shader program %s+%s", vertName, fragName);
	return result;
}

void _programDestroy(ProgramBase* program)
{
	if (!program) return;
	glDeleteProgram(program->id);
	program->id = 0;
	logDebug(applog, u8"Destroyed shader program %s+%s",
		program->vertName, program->fragName);
	free(program);
	program = null;
}

Uniform _programUniform(ProgramBase* program, const char* uniform)
{
	assert(program);
	assert(uniform);
	Uniform result = glGetUniformLocation(program->id, uniform);
	if (result == -1)
		logWarn(applog,
			u8"\"%s\" uniform not available in shader program %s+%s",
			uniform, program->vertName, program->fragName);
	return result;
}

TextureUnit
_programSampler(ProgramBase* program, const char* sampler, TextureUnit unit)
{
	assert(program);
	assert(sampler);
	Uniform uniform = programUniform(program, sampler);
	if (uniform != -1) {
		programUse(program);
		glUniform1i(uniform, unit - GL_TEXTURE0);
	}
	return unit;
}

void _programUse(ProgramBase* program)
{
	assert(program);
	if (program->id == boundProgram) return;
	glUseProgram(program->id);
	boundProgram = program->id;
}
