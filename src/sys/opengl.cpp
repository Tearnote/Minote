/**
 * Implementation of opengl.hpp
 * @file
 */

#include "sys/opengl.hpp"

namespace minote {

namespace detail {

auto compileShaderStage(const GLuint id, const char* const name,
	const char* const source) -> bool
{
		ASSERT(id);
		ASSERT(name);
		ASSERT(source);

	glShaderSource(id, 1, &source, nullptr);
	glCompileShader(id);

	GLint compileStatus = 0;
	glGetShaderiv(id, GL_COMPILE_STATUS, &compileStatus);
	if (compileStatus == GL_FALSE) {
		GLchar infoLog[2048] = "";
		glGetShaderInfoLog(id, 2048, nullptr, infoLog);
		L.error(R"(Shader "%s" failed to compile: %s)", name, infoLog);
		return false;
	}
	L.debug(R"(Shader "%s" compiled)", name);
	return true;
}

}

GLObject::~GLObject()
{
#ifndef NDEBUG
	if (id)
		L.warn(R"(OpenGL object "%s" was never destroyed)", stringOrNull(name));
#endif //NDEBUG
}

void Framebuffer::create(const char* const _name)
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
	samples = Samples::None;
	dirty = true;
	attachments.fill(nullptr);

	L.debug(R"(Framebuffer "%s" destroyed)", name);
	name = nullptr;
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
	ASSERT(detail::getAttachment(src, srcBuffer));
	if (depthStencil) {
		ASSERT(detail::getAttachment(src, Attachment::DepthStencil));
		ASSERT(detail::getAttachment(dst, Attachment::DepthStencil));
	}

	glBindFramebuffer(GL_READ_FRAMEBUFFER, src.id);
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, dst.id);
	glReadBuffer(+srcBuffer);

	const ivec2 blitSize = src.attachments[detail::attachmentIndex(srcBuffer)]->size;
	const GLbitfield mask = GL_COLOR_BUFFER_BIT |
		(depthStencil? GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT : 0);

	glBlitFramebuffer(0, 0, blitSize.x, blitSize.y,
		0, 0, blitSize.x, blitSize.y,
		mask, GL_NEAREST);
}

void Shader::create(char const* _name, const char* vertSrc, const char* fragSrc)
{
	ASSERT(!id);
	ASSERT(_name);
	ASSERT(vertSrc);
	ASSERT(fragSrc);

	const GLuint vert = glCreateShader(GL_VERTEX_SHADER);
#ifndef NDEBUG
	glObjectLabel(GL_SHADER, vert, std::strlen(_name), _name);
#endif //NDEBUG
	defer { glDeleteShader(vert); };
	const GLuint frag = glCreateShader(GL_FRAGMENT_SHADER);
#ifndef NDEBUG
	glObjectLabel(GL_SHADER, frag, std::strlen(_name), _name);
#endif //NDEBUG
	defer { glDeleteShader(frag); };

	if (!detail::compileShaderStage(vert, _name, vertSrc))
		return;
	if (!detail::compileShaderStage(frag, _name, fragSrc))
		return;

	id = glCreateProgram();
#ifndef NDEBUG
	glObjectLabel(GL_PROGRAM, id, std::strlen(_name), _name);
#endif //NDEBUG
	glAttachShader(id, vert);
	glAttachShader(id, frag);

	glLinkProgram(id);
	const GLint linkStatus = [=, this] {
		GLint status = 0;
		glGetProgramiv(id, GL_LINK_STATUS, &status);
		return status;
	}();
	if (linkStatus == GL_FALSE) {
		std::array<GLchar, 2048> infoLog = {};
		glGetProgramInfoLog(id, infoLog.size(), nullptr, infoLog.data());
		L.error(R"(Shader "%s" failed to link: %s)", _name, infoLog.data());
		glDeleteProgram(id);
		id = 0;
		return;
	}

	name = _name;

	setLocations();

	L.info(R"(Shader "%s" created)", name);
}

void Shader::destroy()
{
#ifndef NDEBUG
	if (!id) {
		L.warn("Tried to destroy a shader that has not been created");
		return;
	}
#endif //NDEBUG

	glDeleteProgram(id);
	id = 0;
	L.debug(R"(Shader "%s" destroyed)", name);
	name = nullptr;
}

void Shader::bind() const
{
	ASSERT(id);
	glUseProgram(id);
}

}
