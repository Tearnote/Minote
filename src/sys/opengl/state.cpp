#include "sys/opengl/state.hpp"

#include "base/log.hpp"

namespace minote::detail {

void GLState::setFeature(GLenum const feature, bool const state)
{
	auto& featureState = [=, this]() -> bool& {
		switch (feature) {
		case GL_BLEND:
			return blending;
		case GL_CULL_FACE:
			return culling;
		case GL_DEPTH_TEST:
			return depthTesting;
		case GL_SCISSOR_TEST:
			return scissorTesting;
		case GL_STENCIL_TEST:
			return stencilTesting;
		default:
			L.fail("Unknown rasterizer feature");
		}
	}();
	auto const stateFunc = [=] {
		if (state)
			return glEnable;
		return glDisable;
	}();

	if (state == featureState) return;

	stateFunc(feature);
	featureState = state;
}

void GLState::setBlendingMode(GLBlendingMode const mode)
{
	if (mode.src == blendingMode.src && mode.dst == blendingMode.dst) return;

	glBlendFunc(mode.src, mode.dst);
	blendingMode = mode;
}

void GLState::setDepthFunc(GLenum func)
{
	if (func == depthFunc) return;

	glDepthFunc(func);
	depthFunc = func;
}

void GLState::setScissorBox(AABB<2, i32> const box)
{
	if (box == scissorBox) return;

	glScissor(box.pos.x, box.pos.y, box.size.x, box.size.y);
	scissorBox = box;
}

void GLState::setStencilMode(GLStencilMode const mode)
{
	if (mode.func != stencilMode.func ||
		mode.ref != stencilMode.ref) {
		glStencilFunc(mode.func, mode.ref, 0xFFFF'FFFF);
		stencilMode.func = mode.func;
		stencilMode.ref = mode.ref;
	}

	if (mode.sfail != stencilMode.sfail ||
		mode.dpfail != stencilMode.dpfail ||
		mode.dppass != stencilMode.dppass) {
		glStencilOp(mode.sfail, mode.dpfail, mode.dppass);
		stencilMode.sfail = mode.sfail;
		stencilMode.dpfail = mode.dpfail;
		stencilMode.dppass = mode.dppass;
	}
}

void GLState::setViewport(AABB<2, i32> const box)
{
	if (box == viewport) return;

	glViewport(box.pos.x, box.pos.y, box.size.x, box.size.y);
	viewport = box;
}

void GLState::setColorWrite(bool const state)
{
	if (state == colorWrite) return;

	GLboolean const glState = state? GL_TRUE : GL_FALSE;
	glColorMask(glState, glState, glState, glState);
	colorWrite = state;
}

void GLState::setClearColor(color4 const color)
{
	if (color == clearColor) return;

	glClearColor(color.r, color.g, color.b, color.a);
	clearColor = color;
}

void GLState::setClearDepth(GLclampf const depth)
{
	if (depth == clearDepth) return;

	glClearDepth(depth);
	clearDepth = depth;
}

void GLState::setClearStencil(GLint const stencil)
{
	if (stencil == clearStencil) return;

	glClearStencil(stencil);
	clearStencil = stencil;
}

void GLState::bindBuffer(GLenum const target, GLuint const id)
{
	auto& binding = [=, this]() -> GLuint& {
		switch (target) {
		case GL_ARRAY_BUFFER:
			return vertexbuffer;
		case GL_ELEMENT_ARRAY_BUFFER:
			return elementbuffer;
		case GL_TEXTURE_BUFFER:
			return texturebuffer;
		default:
			L.fail("Unknown buffer type");
		}
	}();

	if (id == binding) return;

	glBindBuffer(target, id);
	binding = id;
}

void GLState::bindVertexArray(GLuint const id)
{
	if (id == vertexarray) return;

	glBindVertexArray(id);
	vertexarray = id;
}

void GLState::setTextureUnit(GLenum const unit)
{
	if (!unit || unit == currentUnit) return;

	glActiveTexture(unit);
	currentUnit = unit;
}

void GLState::bindTexture(GLenum const target, GLuint const id)
{
	size_t const unitIndex = currentUnit - GL_TEXTURE0;
	auto& binding = [=, this]() -> GLuint& {
		switch (target) {
		case GL_TEXTURE_2D:
			return textures[unitIndex].texture2D;
		case GL_TEXTURE_2D_MULTISAMPLE:
			return textures[unitIndex].texture2DMS;
		case GL_TEXTURE_BUFFER:
			return textures[unitIndex].bufferTexture;
		default:
			L.fail("Unknown texture type");
		}
	}();

	if (id == binding) return;

	glBindTexture(target, id);
	binding = id;
}

void GLState::bindRenderbuffer(GLuint const id)
{
	if (id == renderbuffer) return;

	glBindRenderbuffer(GL_RENDERBUFFER, id);
	renderbuffer = id;
}

void GLState::bindFramebuffer(GLenum const target, GLuint const id)
{
	auto& binding = [=, this]() -> GLuint& {
		switch (target) {
		case GL_READ_FRAMEBUFFER:
			return framebufferRead;
		case GL_DRAW_FRAMEBUFFER:
			return framebufferWrite;
		default:
			L.fail("Unknown framebuffer binding");
		}
	}();

	if (id == binding) return;

	glBindFramebuffer(target, id);
	binding = id;
}

void GLState::bindShader(GLuint const id)
{
	if (id == shader) return;

	glUseProgram(id);
	shader = id;
}

void GLState::deleteBuffer(GLenum const target, GLuint const id)
{
	auto& binding = [=, this]() -> GLuint& {
		switch (target) {
		case GL_ARRAY_BUFFER:
			return vertexbuffer;
		case GL_ELEMENT_ARRAY_BUFFER:
			return elementbuffer;
		case GL_TEXTURE_BUFFER:
			return texturebuffer;
		default:
			L.fail("Unknown buffer type");
		}
	}();

	glDeleteBuffers(1, &id);
	if (id == binding)
		binding = 0;
}

void GLState::deleteVertexArray(GLuint const id)
{
	glDeleteVertexArrays(1, &id);
	if (id == vertexarray)
		vertexarray = 0;
}

void GLState::deleteTexture(GLenum const target, GLuint const id)
{
	glDeleteTextures(1, &id);
	for (auto& unit : textures) {
		auto& binding = [=, &unit]() -> GLuint& {
			switch (target) {
			case GL_TEXTURE_2D:
				return unit.texture2D;
			case GL_TEXTURE_2D_MULTISAMPLE:
				return unit.texture2DMS;
			case GL_TEXTURE_BUFFER:
				return unit.bufferTexture;
			default:
				L.fail("Unknown texture type");
			}
		}();
		if (id == binding)
			binding = 0;
	}
}

void GLState::deleteRenderbuffer(GLuint const id)
{
	glDeleteRenderbuffers(1, &id);
	if (id == renderbuffer)
		renderbuffer = 0;
}

void GLState::deleteFramebuffer(GLuint const id)
{
	glDeleteFramebuffers(1, &id);
	if (id == framebufferRead)
		framebufferRead = 0;
	if (id == framebufferWrite)
		framebufferWrite = 0;
}

}
