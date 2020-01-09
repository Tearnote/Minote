/** Generic subsystem for shader creation
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

/**
 * Base struct of Program type. To be a valid Program type usable with below
 * functions, a struct needs to have ::ProgramCommon as its first element.
 */
typedef struct ProgramCommon {
	Program id; ///< ID of the program object
	const char* vertName; ///< Filename of the vertex shader for reference
	const char* fragName; ///< Filename of the fragment shader for reference
} ProgramCommon;

/**
 * Create a new Program of specified valid type. Shaders are compiled, linked
 * and ready for use.
 * @param type A struct type with ::ProgramCommon as its first element
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
    (_programDestroy((ProgramCommon*)(program)))

/**
 * Obtain a uniform location from the program. If it fails, returns -1 and logs
 * a warning.
 * @param program The Program object
 * @param uniform String literal of the uniform to query for
 * @return Uniform location in the program, or -1 on failure
 */
#define programUniform(program, uniform) \
    (_programUniform((ProgramCommon*)(program), (uniform)))

/**
 * Activate a Program for rendering. The same Program stays active for any
 * number of draw calls until changed with another programUse().
 * @param program The Program object
 */
#define programUse(program) \
    (_programUse((ProgramCommon*)(program)))

////////////////////////////////////////////////////////////////////////////////
// Implementation details

void* _programCreate(size_t size, const char* vertName, const char* vertSrc,
	const char* fragName, const char* fragSrc);

void _programDestroy(ProgramCommon* program);

Uniform _programUniform(ProgramCommon* program, const char* uniform);

void _programUse(ProgramCommon* program);

#endif //MINOTE_SHADER_H
