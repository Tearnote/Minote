/**
 * Generic subsystem for shader creation
 * @file
 */

#ifndef MINOTE_SHADER_H
#define MINOTE_SHADER_H

#include "glad/glad.h"
#include <GLFW/glfw3.h>

/// Semantic rename of OpenGL shader program object ID
typedef GLuint Program;

/// Semantic rename of OpenGL uniform location
typedef GLint Uniform;

/// Semantic rename of OpenGL texture unit, for sampler uniforms
typedef GLenum TextureUnit;

/**
 * Base struct of Program type. To be a valid Program type usable with below
 * functions, a struct needs to have ::ProgramBase as its first element.
 */
typedef struct ProgramBase {
	Program id;
	const char* vertName; ///< Filename of the vertex shader for reference
	const char* fragName; ///< Filename of the fragment shader for reference
} ProgramBase;

/**
 * Create a new Program of specified valid type. Shaders are compiled, linked
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
 * Destroy a Program. The pointer becomes invalid and cannot be used again.
 * @param program The Program object
 */
#define programDestroy(program) \
    (_programDestroy((ProgramBase*)(program)))

/**
 * Obtain a uniform location from the program. If it fails, returns -1 and logs
 * a warning.
 * @param program The Program object
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
 * Activate a Program for rendering. The same Program stays active for any
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

#endif //MINOTE_SHADER_H
