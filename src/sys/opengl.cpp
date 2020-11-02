/**
 * Implementation of opengl.hpp
 * @file
 */

#include "sys/opengl.hpp"

namespace minote {

namespace detail {

auto compileShaderStage(GLuint const id, char const* const name,
	char const* const source) -> bool
{
	ASSERT(id);
	ASSERT(name);
	ASSERT(source);

	glShaderSource(id, 1, &source, nullptr);
	glCompileShader(id);

	GLint const compileStatus = [=] {
		GLint status = 0;
		glGetShaderiv(id, GL_COMPILE_STATUS, &status);
		return status;
	}();
	if (compileStatus == GL_FALSE) {
		auto const infoLog = [=] {
			std::array<GLchar, 2048> log = {};
			glGetShaderInfoLog(id, log.size(), nullptr, log.data());
			return log;
		}();
		L.error(R"(Shader "%s" failed to compile: %s)", name, infoLog.data());
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

void VertexArray::create(char const* const _name)
{
	ASSERT(!id);
	ASSERT(_name);

	glGenVertexArrays(1, &id);
#ifndef NDEBUG
	glObjectLabel(GL_VERTEX_ARRAY, id, std::strlen(_name), _name);
#endif //NDEBUG
	name = _name;
	attributes.fill(false);

	L.debug(R"(Vertex array "%s" created)", name);
}

void VertexArray::destroy()
{
#ifndef NDEBUG
	if (!id) {
		L.warn("Tried to destroy a vertex array that has not been created");
		return;
	}
#endif //NDEBUG

	glDeleteVertexArrays(1, &id);
	id = 0;

	L.debug(R"(Vertex array "%s" destroyed)", name);
	name = nullptr;
}

void VertexArray::bind()
{
	ASSERT(id);

	glBindVertexArray(id);
}

void Framebuffer::create(char const* const _name)
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
		L.warn("Tried to destroy a framebuffer that has not been created");
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
		auto const[enabledBuffers, enabledSize] = [this] {
			std::array<GLenum, 16> buffers = {};
			std::size_t size = 0;
			for (std::size_t i = 0; i < 16; i += 1) {
				if (!attachments[i])
					continue;
				buffers[size] = i + GL_COLOR_ATTACHMENT0;
				size += 1;
			}
			return std::make_pair(buffers, size);
		}();

		glDrawBuffers(enabledSize, enabledBuffers.data());

#ifndef NDEBUG
		// Check framebuffer correctness
		if (glCheckFramebufferStatus(GL_DRAW_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
			L.error(R"(Framebuffer "%s" validity check failed)", name);
#endif //NDEBUG

		dirty = false;
	}
}

void Framebuffer::bindRead() const
{
	ASSERT(id);
	ASSERT(!dirty);

	glBindFramebuffer(GL_READ_FRAMEBUFFER, id);
}

void Framebuffer::unbind()
{
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
}

void Framebuffer::blit(Framebuffer& dst, Framebuffer const& src,
	Attachment const srcBuffer, bool const depthStencil)
{
	ASSERT(detail::getAttachment(src, srcBuffer));
	if (depthStencil) {
		ASSERT(detail::getAttachment(src, Attachment::DepthStencil));
		ASSERT(detail::getAttachment(dst, Attachment::DepthStencil));
	}

	src.bindRead();
	dst.bind();
	glReadBuffer(+srcBuffer);

	ivec2 const blitSize = src.attachments[detail::attachmentIndex(srcBuffer)]->size;
	GLbitfield const mask = GL_COLOR_BUFFER_BIT |
		(depthStencil? GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT : 0);

	glBlitFramebuffer(0, 0, blitSize.x, blitSize.y,
		0, 0, blitSize.x, blitSize.y,
		mask, GL_NEAREST);
}

void Shader::create(char const* _name, char const* vertSrc, char const* fragSrc)
{
	ASSERT(!id);
	ASSERT(_name);
	ASSERT(vertSrc);
	ASSERT(fragSrc);

	GLuint const vert = glCreateShader(GL_VERTEX_SHADER);
#ifndef NDEBUG
	glObjectLabel(GL_SHADER, vert, std::strlen(_name), _name);
#endif //NDEBUG
	defer { glDeleteShader(vert); };
	GLuint const frag = glCreateShader(GL_FRAGMENT_SHADER);
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
	GLint const linkStatus = [this] {
		GLint status = 0;
		glGetProgramiv(id, GL_LINK_STATUS, &status);
		return status;
	}();
	if (linkStatus == GL_FALSE) {
		auto const infoLog = [this] {
			std::array<GLchar, 2048> log = {};
			glGetProgramInfoLog(id, log.size(), nullptr, log.data());
			return log;
		}();
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

void BufferSampler::setLocation(Shader const& shader, char const* name,
	TextureUnit _unit)
{
	ASSERT(shader.id);
	ASSERT(name);
	ASSERT(_unit != TextureUnit::None);

	location = glGetUniformLocation(shader.id, name);
	if (location == -1) {
		L.warn(R"(Failed to get location for sampler "%s")", name);
		return;
	}

	shader.bind();
	glUniform1i(location, +_unit - GL_TEXTURE0);
	unit = _unit;
}

}
