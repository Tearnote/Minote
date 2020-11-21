// Minote - sys/opengl/buffer.hpp
// Type-safe wrapper for OpenGL buffer object types

#pragma once

#include "glad/glad.h"
#include "base/array.hpp"
#include "base/util.hpp"
#include "sys/opengl/base.hpp"

namespace minote {

// Generic buffer object. Use one of the template specializations below
template<TriviallyCopyable T, GLenum _target>
struct BufferBase : GLObject {

	using Type = T;
	static constexpr GLenum Target = _target;

	bool dynamic = false;

	bool uploaded = false;

	void create(char const* name, bool dynamic);

	void destroy();

	template<template<TriviallyCopyable, size_t> typename Arr, size_t N>
		requires ArrayContainer<Arr, Type, N>
	void upload(Arr<Type, N> const& data);

	void upload(size_t elements, Type data[]); //TODO remove

	void bind() const;

};

// Buffer object for storing per-vertex data
template<TriviallyCopyable T>
using VertexBuffer = BufferBase<T, GL_ARRAY_BUFFER>;

// Valid underlying type for an element buffer
template<typename T>
concept ElementType =
std::is_same_v<T, u8> ||
	std::is_same_v<T, u16> ||
	std::is_same_v<T, u32>;

// Buffer object for storing elements (vertex indices)
template<ElementType T = u32>
using ElementBuffer = BufferBase<T, GL_ELEMENT_ARRAY_BUFFER>;

}

#include "sys/opengl/buffer.tpp"
