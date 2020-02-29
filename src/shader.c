/**
 * Implementation of shader.h
 * @file
 */

#include "shader.h"

#include <assert.h>
#include "util.h"
#include "log.h"

/// Semantic rename of OpenGL shader object ID
typedef GLuint Shader;

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

/**
 * Create a ::Program instance by compiling and linking a vertex shader and a
 * fragment shader.
 * @param size Size of the struct to create in bytes
 * @param vertName Human-readable name of the vertex shader
 * @param vertSrc GLSL source code of the vertex shader
 * @param fragName Human-readable name of the fragment shader
 * @param fragSrc GLSL source code of the fragment shader
 * @return Newly created ::Program's ID
 */
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

/**
 * Destroy a ::Program instance. The program ID becomes invalid and cannot be
 * used again.
 * @param program ::Program ID to destroy
 */
void _programDestroy(ProgramBase* program)
{
	assert(program);
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
	glUseProgram(program->id);
}
