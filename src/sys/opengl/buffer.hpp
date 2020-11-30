// Minote - sys/opengl/buffer.hpp
// Type-safe wrapper for OpenGL buffer object types

#pragma once

#include "glad/glad.h"
#include "base/concept.hpp"
#include "base/array.hpp"
#include "base/util.hpp"
#include "sys/opengl/base.hpp"

namespace minote {

// Generic buffer object. Use one of the template specializations below
template<copy_constructible T, GLenum _target>
struct BufferBase : GLObject {

	// Element type that is being stored by the buffer
	using Type = T;

	// The GLenum value of the buffer's binding target
	static constexpr GLenum Target = _target;

	// Whether the buffer data can be uploaded more than once
	bool dynamic = false;

	// Whether there has been at least one data upload
	bool uploaded = false;

	// Create the buffer object; the storage is empty by default. Set dynamic
	// to true if you want to upload data more than once (streaming buffer.)
	void create(char const* name, bool dynamic);

	// Clean up the buffer, freeing memory on the GPU.
	void destroy();

	// Upload new data to the GPU buffer, replacing previous data. The buffer
	// is resized to fit the new data, and the previous storage is orphaned.
	void upload(span<Type const> data);

	// Bind the buffer to the Target binding point.
	void bind() const;

};

// Buffer object for storing per-vertex data (VBO)
template<copy_constructible T>
using VertexBuffer = BufferBase<T, GL_ARRAY_BUFFER>;

// Valid underlying type for an element buffer
template<typename T>
concept ElementType =
	std::is_same_v<T, GLubyte> ||
	std::is_same_v<T, GLushort> ||
	std::is_same_v<T, GLuint>;

// Buffer object for storing vertex indices (EBO)
template<ElementType T = GLuint>
using ElementBuffer = BufferBase<T, GL_ELEMENT_ARRAY_BUFFER>;

}

#include "sys/opengl/buffer.tpp"
