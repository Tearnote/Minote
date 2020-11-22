// Minote - sys/opengl/vertexarray.hpp
// OpenGL vertex array object (VAO) wrapper

#pragma once

#include "glad/glad.h"
#include "base/array.hpp"
#include "sys/opengl/buffer.hpp"
#include "sys/opengl/base.hpp"

namespace minote {

// Vertex array object. Aggregate object that holds the definition of a vertex
// layout for use in a shader.
struct VertexArray : GLObject {

	// List of vertex attributes; true when the attribute pointer is set
	array<bool, 16> attributes = {};

	// 0 if no element buffer (EBO) is bound; otherwise the size of index type
	// in bits
	size_t elementBits = 0;

	// Create the VAO object. All attribute bindings are initially empty.
	void create(char const* name);

	// Clean up the VAO object. Buffers bound to attributes are unaffected.
	void destroy();

	// Set an attribute to a VBO pointer. The VBO must be storing a GLSL type.
	// Set instanced to true to advance the pointer per instance instead of
	// per vertex. mat4 attributes take up 4 indexes, from index to index+3.
	template<GLSLType T>
	void setAttribute(GLuint index, VertexBuffer<T>& buffer, bool instanced = false);

	// Set an attribute to a VBO field pointer. The VBO must be storing a struct
	// of GLSL types. Set instanced to true to advance the pointer per instance
	// instead of per vertex. mat4 attributes take up 4 indexes,
	// from index to index+3.
	template<TriviallyCopyable T, GLSLType U>
	void setAttribute(GLuint index, VertexBuffer<T>& buffer, U T::*field, bool instanced = false);

	// Set the element buffer binding.
	template<ElementType T>
	void setElements(ElementBuffer<T>& buffer);

	// Bind the VAO, activating the vertex definition for subsequent drawcalls.
	void bind();

};

}

#include "sys/opengl/vertexarray.tpp"
