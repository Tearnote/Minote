// Minote - sys/opengl/state.hpp
// Internal OpenGL state tracker

#pragma once

#include "glad/glad.h"
#include "base/array.hpp"
#include "base/math.hpp"

namespace minote::detail {

struct GLState {

	struct GLBlendingMode {

		GLenum src = GL_ONE;
		GLenum dst = GL_ZERO;

	};

	struct GLTextureUnitState {

		GLuint texture2D = 0;
		GLuint texture2DMS = 0;
		GLuint bufferTexture = 0;

	};

	struct GLStencilMode {

		GLenum func = GL_ALWAYS;
		GLint ref = 0;

		GLenum sfail = GL_KEEP;
		GLenum dpfail = GL_KEEP;
		GLenum dppass = GL_KEEP;

	};

	// Rasterizer features
	bool blending = false;
	GLBlendingMode blendingMode;
	bool culling = false;
	bool depthTesting = false;
	GLenum depthFunc = GL_LESS;
	bool scissorTesting = false;
	AABB<2, i32> scissorBox;
	bool stencilTesting = false;
	GLStencilMode stencilMode;
	AABB<2, i32> viewport;
	bool colorWrite = true;
	color4 clearColor = {0.0f, 0.0f, 0.0f, 0.0f};
	GLclampf clearDepth = 1.0f;
	GLint clearStencil = 0;

	void setFeature(GLenum feature, bool state);

	void setBlendingMode(GLBlendingMode mode);

	void setDepthFunc(GLenum func);

	void setScissorBox(AABB<2, i32> box);

	void setStencilMode(GLStencilMode mode);

	void setViewport(AABB<2, i32> box);

	void setColorWrite(bool state);

	void setClearColor(color4 color);

	void setClearDepth(GLclampf depth);

	void setClearStencil(GLint stencil);

	// Object bindings
	GLuint vertexbuffer = 0;
	GLuint elementbuffer = 0;
	GLuint texturebuffer = 0;
	GLuint vertexarray = 0;
	GLenum currentUnit = GL_TEXTURE0;
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

	// Object deletion
	void deleteBuffer(GLenum target, GLuint id);

	void deleteVertexArray(GLuint id);

	void deleteTexture(GLenum target, GLuint id);

	void deleteRenderbuffer(GLuint id);

	void deleteFramebuffer(GLuint id);

};

inline thread_local GLState state;

}
