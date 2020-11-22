// Minote - sys/opengl/draw.hpp
// Wrapper for a complete OpenGL drawcall state

#pragma once

#include "glad/glad.h"
#include "base/math.hpp"
#include "sys/opengl/vertexarray.hpp"
#include "sys/opengl/shader.hpp"
#include "sys/opengl/framebuffer.hpp"

namespace minote {

// Method of forming primitives out of vertices
enum struct DrawMode : GLenum {
	Triangles = GL_TRIANGLES,
	TriangleStrip = GL_TRIANGLE_STRIP
};

// A method of comparing two values
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

// Available blending operations
enum struct BlendingOp : GLenum {
	Zero = GL_ZERO,
	One = GL_ONE,
	SrcAlpha = GL_SRC_ALPHA,
	OneMinusSrcAlpha = GL_ONE_MINUS_SRC_ALPHA
};

// Action to take on a stencil value
enum struct StencilOp : GLenum {
	Nothing = GL_KEEP,
	Clear = GL_ZERO,
	Set = GL_REPLACE, // to reference value
	Increment = GL_INCR,
	Decrement = GL_DECR,
	Invert = GL_INVERT
};

// Desired rasterized state. The defaults are set to be reasonable so that
// as few settings need to be changed as possible in typical usage.
// Most conveniently created with designated initializers.
struct DrawParams {

	// Whether blending operations will be applied
	bool blending = false;

	struct BlendingMode {

		// Operation to take on the source (new) fragment
		BlendingOp src;

		// Operation to take on the destination (existing) fragment
		BlendingOp dst;

	} blendingMode = { BlendingOp::SrcAlpha, BlendingOp::OneMinusSrcAlpha };

	// Whether backface culling will be performed
	// Winding order is the default value of CCW
	bool culling = true;

	// Whether depth testing will be performed
	bool depthTesting = true;

	// Condition that needs to be met by the fragment to pass the depth test
	Comparison depthFunc = Comparison::LesserEqual;

	// Whether scissor testing will be performed
	bool scissorTesting = false;

	// The area that passes the scissor test, in pixels
	// The origin is bottom left
	AABB<2, i32> scissorBox;

	// Whether stencil testing will be performed
	bool stencilTesting = false;

	struct StencilMode {

		// Condition that needs to be met by the stencil value against
		// the reference value to pass the stencil test
		Comparison func = Comparison::Equal;

		// Reference value
		i32 ref = 0;

		// Action to take if the stencil test fails
		StencilOp sfail = StencilOp::Nothing;

		// Action to take if the stencil test passes, but depth test fails
		StencilOp dpfail = StencilOp::Nothing;

		// Action to take if both the stencil test and the depth test pass
		StencilOp dppass = StencilOp::Nothing;
	} stencilMode;

	// Size of the rendering viewport
	AABB<2, i32> viewport;

	// Whether to write color values. Disable to do stencil- or depth- only
	// writes
	bool colorWrite = true;

	// Apply the minimal set of OpenGL state changes required to achieve
	// the desired rasterizer state.
	void set() const;

};

// A complete description of a drawcall, encapsulated to be independent
// from past OpenGL state; separate Draw objects will not affect each other.
// The shader type parameter is provided so that shader uniforms can be
// conveniently accessed from the Draw object.
// Most easily filled in with a designated initializer.
template<ShaderType T = Shader>
struct Draw {

	// Optional shader program to use. If nullptr, no primitive will be drawn
	T* shader = nullptr;

	// Optional vertex array. If nullptr, no VAO will be used
	VertexArray* vertexarray = nullptr;

	// Optional framebuffer. If nullptr, will draw into the backbuffer
	Framebuffer* framebuffer = nullptr;

	// Method of forming primitives from vertices
	DrawMode mode = DrawMode::Triangles;

	// Number of triangles to draw
	GLsizei triangles = 0;

	// Number of instances to draw
	GLsizei instances = 1;

	// Index of the first vertex
	GLint offset = 0;

	// Desired rasterizer state
	DrawParams params;

	// Whether to clear the color buffer(s) before drawing
	bool clearColor = false;

	// Whether to clear the DS buffer before drawing
	bool clearDepthStencil = false;

	struct ClearParams {

		// Color value to fill the color buffer with
		color4 color = {0.0f, 0.0f, 0.0f, 1.0f};

		// Depth value to fill the DS buffer with
		f32 depth = 1.0f;

		// Stencil value to fill the DS buffer with
		u8 stencil = 0;
	} clearParams;

	// Execute the drawcall according to values set in the object.
	void draw();

};

}

#include "sys/opengl/draw.tpp"
