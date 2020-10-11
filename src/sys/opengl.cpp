/**
 * Implementation of opengl.hpp
 * @file
 */

#include "sys/opengl.hpp"

#include "base/log.hpp"

namespace minote {

/// OpenGL shader object ID
using Shader = GLuint;

GLuint boundProgram = 0;

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

static auto attachmentIndex(const Attachment attachment) -> std::size_t
{
	switch(attachment) {
	case Attachment::DepthStencil:
		return 16;
	case Attachment::None:
		L.warn("Invalid attachment index %d", +attachment);
		return -1;
	default:
		return (+attachment) - (+Attachment::Color0);
	}
}

/**
 * Helper function to retrieve a texture pointer at a specified attachment point
 * @param f Framebuffer object
 * @param attachment Attachment point
 * @return The pointer to texture at given attachment point (can be nullptr)
 */
static auto getAttachment(Framebuffer& f, const Attachment attachment) -> const TextureBase*&
{
	return f.attachments[attachmentIndex(attachment)];
}

static auto getAttachment(const Framebuffer& f, const Attachment attachment) -> const TextureBase* const&
{
	return f.attachments[attachmentIndex(attachment)];
}

void Texture::create(const char* _name, size2i _size, PixelFormat _format)
{
	ASSERT(!id);
	ASSERT(_name);
	ASSERT(_format != PixelFormat::None);

	glGenTextures(1, &id);
#ifndef NDEBUG
	glObjectLabel(GL_TEXTURE, id, std::strlen(_name), _name);
#endif //NDEBUG
	name = _name;
	glBindTexture(GL_TEXTURE_2D, id);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	setFilter(Filter::Linear);
	format = _format;
	resize(_size);

	L.debug(R"(Texture "%s" created)", name);
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

	L.debug(R"(Texture "%s" destroyed)", name);
	name = nullptr;
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

void TextureMS::create(const char* _name, size2i _size, PixelFormat _format, GLsizei _samples)
{
	ASSERT(!id);
	ASSERT(_name);
	ASSERT(_format != PixelFormat::None);
	ASSERT(_samples == 2 || _samples == 4 || _samples == 8);

	glGenTextures(1, &id);
#ifndef NDEBUG
	glObjectLabel(GL_TEXTURE, id, std::strlen(_name), _name);
#endif //NDEBUG
	name = _name;
	format = _format;
	samples = _samples;
	resize(_size);

	L.debug(R"(Multisample texture "%s" created)", name);
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

	L.debug(R"(Multisample texture "%s" destroyed)", name);
	name = nullptr;
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

void Renderbuffer::create(const char* _name, size2i _size, PixelFormat _format)
{
	ASSERT(!id);
	ASSERT(_name);
	ASSERT(_format != PixelFormat::None);

	glGenRenderbuffers(1, &id);
#ifndef NDEBUG
	glObjectLabel(GL_RENDERBUFFER, id, std::strlen(_name), _name);
#endif //NDEBUG
	name = _name;
	format = _format;
	resize(_size);

	L.debug(R"(Renderbuffer "%s" created)", name);
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

	L.debug(R"(Renderbuffer "%s" destroyed)", name);
	name = nullptr;
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

void RenderbufferMS::create(const char* _name, size2i _size, PixelFormat _format, GLsizei _samples)
{
	ASSERT(!id);
	ASSERT(_name);
	ASSERT(_format != PixelFormat::None);
	ASSERT(_samples == 2 || _samples == 4 || _samples == 8);

	glGenRenderbuffers(1, &id);
#ifndef NDEBUG
	glObjectLabel(GL_RENDERBUFFER, id, std::strlen(_name), _name);
#endif //NDEBUG
	name = _name;
	format = _format;
	samples = _samples;
	resize(_size);

	L.debug(R"(Multisample renderbuffer "%s" created)", name);
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

	L.debug(R"(Multisample renderbuffer "%s" destroyed)", name);
	name = nullptr;
}

void RenderbufferMS::resize(size2i _size)
{
	ASSERT(_size.x > 0 && _size.y > 0);
	ASSERT(id);
	if (size == _size)
		return;

	glBindRenderbuffer(GL_RENDERBUFFER, id);
	glRenderbufferStorageMultisample(GL_RENDERBUFFER, samples, +format,
		_size.x, _size.y);
	size = _size;
}

void Framebuffer::create(const char* _name)
{
	ASSERT(!id);
	ASSERT(_name);
	glGenFramebuffers(1, &id);
#ifndef NDEBUG
	glObjectLabel(GL_FRAMEBUFFER, id, std::strlen(_name), _name);
#endif //NDEBUG
	name = _name;

	L.debug(R"(Framebuffer "%s" created)", name);
}

void Framebuffer::destroy()
{
#ifndef NDEBUG
	if (!id) {
		L.warn("Tried to destroy a multisample renderbuffer that has not been created");
		return;
	}
#endif //NDEBUG

	glDeleteFramebuffers(1, &id);
	id = 0;
	samples = -1;
	dirty = true;
	attachments.fill(nullptr);

	L.debug(R"(Framebuffer "%s" destroyed)", name);
	name = nullptr;
}

void Framebuffer::attach(const Texture& t, const Attachment attachment)
{
	ASSERT(id);
	ASSERT(t.id);
	ASSERT(attachment != Attachment::None);
#ifndef NDEBUG
	if (t.format == PixelFormat::DepthStencil)
		ASSERT(attachment == Attachment::DepthStencil);
	else
		ASSERT(attachment != Attachment::DepthStencil);
	if (samples != -1)
		ASSERT(samples == 0);
	ASSERT(!getAttachment(*this, attachment));
#endif //NDEBUG

	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, id);
	glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, +attachment,
		GL_TEXTURE_2D, t.id, 0);
	getAttachment(*this, attachment) = &t;
	samples = 0;
	dirty = true;

	L.debug(R"(Texture "%s" attached to framebuffer "%s")", t.name, name);
}

void Framebuffer::attach(const TextureMS& t, Attachment attachment)
{
	ASSERT(id);
	ASSERT(t.id);
	ASSERT(attachment != Attachment::None);
#ifndef NDEBUG
	if (t.format == PixelFormat::DepthStencil)
		ASSERT(attachment == Attachment::DepthStencil);
	else
		ASSERT(attachment != Attachment::DepthStencil);
	if (samples != -1)
		ASSERT(samples == t.samples);
#endif //NDEBUG

	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, id);
	glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, +attachment,
		GL_TEXTURE_2D_MULTISAMPLE, t.id, 0);
	getAttachment(*this, attachment) = &t;
	samples = t.samples;
	dirty = true;

	L.debug(R"(Multisample texture "%s" attached to framebuffer "%s")", t.name, name);
}

void Framebuffer::attach(const Renderbuffer& r, Attachment attachment)
{
	ASSERT(id);
	ASSERT(r.id);
	ASSERT(attachment != Attachment::None);
#ifndef NDEBUG
	if (r.format == PixelFormat::DepthStencil)
		ASSERT(attachment == Attachment::DepthStencil);
	else
		ASSERT(attachment != Attachment::DepthStencil);
	if (samples != -1)
		ASSERT(samples == 0);
#endif //NDEBUG

	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, id);
	glFramebufferRenderbuffer(GL_DRAW_FRAMEBUFFER, +attachment,
		GL_RENDERBUFFER, r.id);
	getAttachment(*this, attachment) = &r;
	samples = 0;
	dirty = true;

	L.debug(R"(Renderbuffer "%s" attached to framebuffer "%s")", r.name, name);
}

void Framebuffer::attach(const RenderbufferMS& r, Attachment attachment)
{
	ASSERT(id);
	ASSERT(r.id);
	ASSERT(attachment != Attachment::None);
#ifndef NDEBUG
	if (r.format == PixelFormat::DepthStencil)
		ASSERT(attachment == Attachment::DepthStencil);
	else
		ASSERT(attachment != Attachment::DepthStencil);
	if (samples != -1)
		ASSERT(samples == r.samples);
#endif //NDEBUG

	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, id);
	glFramebufferRenderbuffer(GL_DRAW_FRAMEBUFFER, +attachment,
		GL_RENDERBUFFER, r.id);
	getAttachment(*this, attachment) = &r;
	samples = r.samples;
	dirty = true;

	L.debug(R"(Multisample renderbuffer "%s" attached to framebuffer "%s")", r.name, name);
}

void Framebuffer::bind()
{
	ASSERT(id);

	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, id);

	if (dirty) {
		// Call glDrawBuffers with all enabled color framebuffers
		std::array<GLenum, 16> enabledBuffers = {};
		std::size_t enabledSize = 0;
		for (std::size_t i = 0; i < 16; i += 1) {
			if (!attachments[i])
				continue;
			enabledBuffers[enabledSize] = i + GL_COLOR_ATTACHMENT0;
			enabledSize += 1;
		}
		L.trace(R"(Framebuffer "%s" verified with %zu color attachments)", name, enabledSize);
		glDrawBuffers(enabledSize, enabledBuffers.data());

#ifndef NDEBUG
		// Check framebuffer correctness
		if (glCheckFramebufferStatus(GL_DRAW_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
			L.error(R"(Framebuffer "%s" validity check failed)", name);
#endif //NDEBUG

		dirty = false;
	}
}

void Framebuffer::unbind()
{
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
}

void Framebuffer::blit(Framebuffer& dst, const Framebuffer& src,
	const Attachment srcBuffer, const bool depthStencil)
{
	ASSERT(getAttachment(src, srcBuffer));
	if (depthStencil) {
		ASSERT(getAttachment(src, Attachment::DepthStencil));
		ASSERT(getAttachment(dst, Attachment::DepthStencil));
	}

	glBindFramebuffer(GL_READ_FRAMEBUFFER, src.id);
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, dst.id);
	glReadBuffer(+srcBuffer);

	const size2i blitSize = src.attachments[attachmentIndex(srcBuffer)]->size;
	const GLbitfield mask = GL_COLOR_BUFFER_BIT |
		(depthStencil? GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT : 0);

	glBlitFramebuffer(0, 0, blitSize.x, blitSize.y,
		0, 0, blitSize.x, blitSize.y,
		mask, GL_NEAREST);
}

}
