// Minote - sys/opengl/state.hpp
// Internal OpenGL state tracker. Used by other wrappers to minimize OpenGL
// calls.

#pragma once

#include "glad/glad.h"
#include "base/array.hpp"
#include "base/math.hpp"

namespace minote::detail {

// Cached OpenGL state. Contains rasterizer state and binding points
struct GLState {

	// *** Rasterizer features ***

	// Whether blending is enabled
	bool blending = false;

	struct GLBlendingMode {

		// Operation to perform on the source fragment
		GLenum src = GL_ONE;

		// Operation to perform on the destination fragment
		GLenum dst = GL_ZERO;

	} blendingMode;

	// Whether backface culling is enabled (CCW winding order left as default)
	bool culling = false;

	// Whether depth testing is enabled
	bool depthTesting = false;

	// Condition that needs to be met by the fragment to pass the depth test
	GLenum depthFunc = GL_LESS;

	// Whether scissor testing is enabled
	bool scissorTesting = false;

	// The area that passes the scissor test, in pixels
	AABB<2, i32> scissorBox = {};

	// Whether stencil testing is enabled
	bool stencilTesting = false;

	struct GLStencilMode {

		// Condition that needs to be met by the stencil value against
		// the reference value to pass the stencil test
		GLenum func = GL_ALWAYS;

		// Reference value
		GLint ref = 0;

		// Action to take if the stencil test fails
		GLenum sfail = GL_KEEP;

		// Action to take if the stencil test passes, but depth test fails
		GLenum dpfail = GL_KEEP;

		// Action to take if both the stencil test and the depth test pass
		GLenum dppass = GL_KEEP;

	} stencilMode;

	// Size of the rendering viewport
	AABB<2, i32> viewport = {};

	// Whether writing to color buffers is enabled
	bool colorWrite = true;

	// Color value to fill the color buffer with on glClear
	color4 clearColor = {0.0f, 0.0f, 0.0f, 0.0f};

	// Depth value to fill the DS buffer with on glClear
	GLclampf clearDepth = 1.0f;

	// Stencil value to fill the DS buffer with on glClear
	GLint clearStencil = 0;

	// Ensure the state of a specific rasterizer feature. OpenGL enumerators
	// such as GL_BLEND are accepted.
	void setFeature(GLenum feature, bool state);

	// Change the blending mode if needed.
	void setBlendingMode(GLBlendingMode mode);

	// Change the depth test comparison method if needed.
	void setDepthFunc(GLenum func);

	// Set the scissor test area if needed.
	void setScissorBox(AABB<2, i32> box);

	// Set the stencil test condition and actions if needed.
	void setStencilMode(GLStencilMode mode);

	// Set the rendering viewport if needed.
	void setViewport(AABB<2, i32> box);

	// Change whether color buffer writing is enabled if needed.
	void setColorWrite(bool state);

	// Change the glClear color value if needed.
	void setClearColor(color4 color);

	// Change the glClear depth value if needed.
	void setClearDepth(GLclampf depth);

	// Change the glClear stencil value if needed.
	void setClearStencil(GLint stencil);

	// *** Object bindings ***
	// Fields and methods are self-explanatory.

	GLuint vertexbuffer = 0;
	GLuint elementbuffer = 0;
	GLuint texturebuffer = 0;
	GLuint vertexarray = 0;
	GLenum currentUnit = GL_TEXTURE0;

	struct GLTextureUnitState {

		GLuint texture2D = 0;
		GLuint texture2DMS = 0;
		GLuint bufferTexture = 0;

	};
	array<GLTextureUnitState, 16> textures;
	GLuint renderbuffer = 0;
	GLuint framebufferRead = 0;
	GLuint framebufferWrite = 0;
	GLuint shader = 0;

	void bindBuffer(GLenum target, GLuint id);

	void bindVertexArray(GLuint id);

	void setTextureUnit(GLenum unit);

	void bindTexture(GLenum target, GLuint id);

	void bindRenderbuffer(GLuint id);

	void bindFramebuffer(GLenum target, GLuint id);

	void bindShader(GLuint id);

	// *** Object deletion ***
	// In OpenGL, deleting objects can affect global state if the object
	// being deleted is currently bound. Use these wrappers to ensure that
	// the cached state is correct.

	void deleteBuffer(GLenum target, GLuint id);

	void deleteVertexArray(GLuint id);

	void deleteTexture(GLenum target, GLuint id);

	void deleteRenderbuffer(GLuint id);

	void deleteFramebuffer(GLuint id);

};

// Global OpenGL state cache
inline thread_local GLState state;

}
