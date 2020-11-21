// Minote - sys/opengl/vertexarray.hpp
// OpenGL vertex array object (VAO) wrapper

#pragma once

#include "glad/glad.h"
#include "base/array.hpp"
#include "sys/opengl/buffer.hpp"
#include "sys/opengl/base.hpp"

namespace minote {

struct VertexArray : GLObject {

	array<bool, 16> attributes = {};
	size_t elementBits = 0;

	void create(char const* name);

	void destroy();

	template<GLSLType T>
	void setAttribute(GLuint index, VertexBuffer<T>& buffer, bool instanced = false);

	template<TriviallyCopyable T, GLSLType U>
	void setAttribute(GLuint index, VertexBuffer<T>& buffer, U T::*field, bool instanced = false);

	template<ElementType T>
	void setElements(ElementBuffer<T>& buffer);

	void bind();

};

}

#include "sys/opengl/vertexarray.tpp"
