// Minote - sys/opengl/draw.hpp
// Wrapper for a complete OpenGL drawcall state

#pragma once

#include "glad/glad.h"
#include "base/math.hpp"
#include "sys/opengl/vertexarray.hpp"
#include "sys/opengl/shader.hpp"
#include "sys/opengl/framebuffer.hpp"

namespace minote {

enum struct DrawMode : GLenum {
	Triangles = GL_TRIANGLES,
	TriangleStrip = GL_TRIANGLE_STRIP
};

enum struct Comparison : GLenum {
	Never = GL_NEVER,
	Always = GL_ALWAYS,
	Equal = GL_EQUAL,
	Inequal = GL_NOTEQUAL,
	Lesser = GL_LESS,
	Greater = GL_GREATER,
	LesserEqual = GL_LEQUAL,
	GreaterEqual = GL_GEQUAL
};

enum struct BlendingOp : GLenum {
	Zero = GL_ZERO,
	One = GL_ONE,
	SrcAlpha = GL_SRC_ALPHA,
	OneMinusSrcAlpha = GL_ONE_MINUS_SRC_ALPHA
};

enum struct StencilOp : GLenum {
	Nothing = GL_KEEP,
	Clear = GL_ZERO,
	Set = GL_REPLACE,
	Increment = GL_INCR,
	Decrement = GL_DECR,
	Invert = GL_INVERT
};

struct DrawParams {

	bool blending = false;
	struct BlendingMode {
		BlendingOp src;
		BlendingOp dst;
	} blendingMode = { BlendingOp::SrcAlpha, BlendingOp::OneMinusSrcAlpha };

	bool culling = true;

	bool depthTesting = true;
	Comparison depthFunc = Comparison::LesserEqual;

	bool scissorTesting = false;
	AABB<2, i32> scissorBox;

	bool stencilTesting = false;
	struct StencilMode {
		Comparison func = Comparison::Equal;
		i32 ref = 0;
		StencilOp sfail = StencilOp::Nothing;
		StencilOp dpfail = StencilOp::Nothing;
		StencilOp dppass = StencilOp::Nothing;
	} stencilMode;

	AABB<2, i32> viewport;

	bool colorWrite = true;

	void set() const;

};

template<ShaderType T = Shader>
struct Draw {

	T* shader = nullptr;
	VertexArray* vertexarray = nullptr;
	Framebuffer* framebuffer = nullptr;
	DrawMode mode = DrawMode::Triangles;
	GLsizei triangles = 0;
	GLsizei instances = 1;
	GLint offset = 0;
	DrawParams params;

	bool clearColor = false;
	bool clearDepthStencil = false;
	struct ClearParams {
		color4 color = {0.0f, 0.0f, 0.0f, 1.0f};
		f32 depth = 1.0f;
		u8 stencil = 0;
	} clearParams;

	void draw();

};

}

#include "sys/opengl/draw.tpp"
