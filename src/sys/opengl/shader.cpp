#include "sys/opengl/shader.hpp"

#include "base/log.hpp"

namespace minote {

// Compile a single shader stage. Returns false if compilation failed.
static auto compileShaderStage(GLuint const id, char const* const name,
	char const* const source) -> bool
{
	DASSERT(id);
	DASSERT(name);
	DASSERT(source);

	glShaderSource(id, 1, &source, nullptr);
	glCompileShader(id);

	GLint const compileStatus = [=] {
		GLint status = 0;
		glGetShaderiv(id, GL_COMPILE_STATUS, &status);
		return status;
	}();
	if (compileStatus == GL_FALSE) {
		auto const infoLog = [=] {
			array<GLchar, 2048> log = {};
			glGetShaderInfoLog(id, log.size(), nullptr, log.data());
			return log;
		}();
		L.error(R"(Shader "{}" failed to compile: {})", name, infoLog.data());
		return false;
	}
	return true;
}

void Shader::create(char const* _name, char const* vertSrc, char const* fragSrc)
{
	DASSERT(!id);
	DASSERT(_name);
	DASSERT(vertSrc);
	DASSERT(fragSrc);

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

	if (!compileShaderStage(vert, _name, vertSrc))
		return;
	if (!compileShaderStage(frag, _name, fragSrc))
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
			array<GLchar, 2048> log = {};
			glGetProgramInfoLog(id, log.size(), nullptr, log.data());
			return log;
		}();
		L.error(R"(Shader "{}" failed to link: {})", _name, infoLog.data());
		glDeleteProgram(id);
		id = 0;
		return;
	}

	name = _name;

	setLocations();

	L.info(R"(Shader "{}" created)", name);
}

void Shader::destroy()
{
	DASSERT(id);

	glDeleteProgram(id);
	id = 0;
	L.debug(R"(Shader "{}" destroyed)", name);
	name = nullptr;
}

void Shader::bind() const
{
	DASSERT(id);
	detail::state.bindShader(id);
}

void BufferSampler::setLocation(Shader const& shader, char const* name,
	TextureUnit _unit)
{
	DASSERT(shader.id);
	DASSERT(name);
	DASSERT(_unit != TextureUnit::None);

	location = glGetUniformLocation(shader.id, name);
	if (location == -1) {
		L.warn(R"(Failed to get location for sampler "{}")", name);
		return;
	}

	shader.bind();
	glUniform1i(location, +_unit - GL_TEXTURE0);
	unit = _unit;
}

}
