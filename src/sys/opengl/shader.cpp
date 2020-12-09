#include "sys/opengl/shader.hpp"

#include "glad/glad.h"
#include "base/array.hpp"
#include "base/util.hpp"
#include "base/io.hpp"
#include "sys/opengl/state.hpp"

namespace minote {

// Compile a single shader stage. Throws if compilation failed.
static void compileShaderStage(u32 id, string_view name, string_view source) {
	ASSERT(id);

	// Perform compilation
	auto const* const str{source.data()};
	auto const strLen{static_cast<i32>(source.size())};
	glShaderSource(id, 1, &str, &strLen);
	glCompileShader(id);

	// Check for compilation errors
	i32 compileStatus;
	glGetShaderiv(id, GL_COMPILE_STATUS, &compileStatus);
	if (compileStatus == GL_FALSE) {
		i32 infoLogLength;
		glGetShaderiv(id, GL_INFO_LOG_LENGTH, &infoLogLength);
		string infoLog;
		infoLog.reserve(infoLogLength);
		glGetShaderInfoLog(id, infoLog.size(), nullptr, infoLog.data());
		throw runtime_error{format(R"(Shader "{}" failed to compile: {})", name, infoLog)};
	}
}

Shader::Uniform::Uniform(Shader& _parent, string_view name, u32 type, size_t _size):
	parent{_parent}, size{_size} {
	string uname{name};
	location = glGetUniformLocation(parent.id, uname.c_str());
	if (location == -1)
		throw runtime_error{format(R"(Failed to get location for uniform "{}")", name)};

	switch (type) {
	case GL_FLOAT: value.emplace<f32>(); break;
	case GL_FLOAT_VEC2: value.emplace<vec2>(); break;
	case GL_FLOAT_VEC3: value.emplace<vec3>(); break;
	case GL_FLOAT_VEC4: value.emplace<vec4>(); break;
	default: throw logic_error{format("Unknown uniform type enum {}", type)};
	}
}

Shader::Shader(string_view _name, string_view vertSrc, string_view fragSrc): name{_name} {
	// Create individual shader stage objects
	u32 const vert = glCreateShader(GL_VERTEX_SHADER);
#ifndef NDEBUG
	glObjectLabel(GL_SHADER, vert, name.size(), name.c_str());
#endif //NDEBUG
	defer { glDeleteShader(vert); };
	u32 const frag = glCreateShader(GL_FRAGMENT_SHADER);
#ifndef NDEBUG
	glObjectLabel(GL_SHADER, frag, name.size(), name.c_str());
#endif //NDEBUG
	defer { glDeleteShader(frag); };

	// Compile individual shader stages
	compileShaderStage(vert, name, vertSrc);
	compileShaderStage(frag, name, fragSrc);

	// Link shader program
	id = glCreateProgram();
#ifndef NDEBUG
	glObjectLabel(GL_PROGRAM, id, name.size(), name.c_str());
#endif //NDEBUG
	glAttachShader(id, vert);
	glAttachShader(id, frag);
	glLinkProgram(id);

	// Check for linking errors
	i32 linkStatus;
	glGetProgramiv(id, GL_LINK_STATUS, &linkStatus);
	if (linkStatus == GL_FALSE) {
		i32 infoLogLength;
		glGetProgramiv(id, GL_INFO_LOG_LENGTH, &infoLogLength);
		string infoLog;
		infoLog.reserve(infoLogLength);
		glGetProgramInfoLog(id, infoLog.size(), nullptr, infoLog.data());
		glDeleteProgram(id);
		throw runtime_error{format(R"(Shader "{}" failed to link: {})", name, infoLog.data())};
	}

	// Query for uniforms
	i32 uniformCount;
	glGetProgramiv(id, GL_ACTIVE_UNIFORMS, &uniformCount);
	i32 uniformNameLength;
	glGetProgramiv(id, GL_ACTIVE_UNIFORM_MAX_LENGTH, &uniformNameLength);
	for (size_t i: iota_view{0, uniformCount}) {
		i32 size;
		u32 type;
		string uname;
		uname.reserve(uniformNameLength);
		glGetActiveUniform(id, i, uniformNameLength, nullptr, &size, &type, uname.data());
		uniforms.emplace(uname, Uniform{*this, uname, type, static_cast<size_t>(size)});
	}
}

Shader::~Shader() {
	if (!id) return;
	glDeleteProgram(id);
}

void Shader::bind() const {
	detail::state.bindShader(id);
}

}
