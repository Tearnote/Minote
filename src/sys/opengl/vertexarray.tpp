#pragma once

#include "base/util.hpp"
#include "base/log.hpp"
#include "sys/opengl/state.hpp"

namespace minote {

namespace detail {

// Set the VAO attribute to a vertex buffer pointer.
template<GLSLType Component, typename T>
auto setVaoAttribute(VertexArray& vao, GLuint const index,
	VertexBuffer<T>& buffer, std::ptrdiff_t const offset, bool const instanced)
{
	constexpr auto components = []() -> GLint {
		if constexpr (std::is_same_v<Component, vec2> ||
			std::is_same_v<Component, ivec2> ||
			std::is_same_v<Component, uvec2>)
			return 2;
		if constexpr (std::is_same_v<Component, vec3> ||
			std::is_same_v<Component, ivec3> ||
			std::is_same_v<Component, uvec3>)
			return 3;
		if constexpr (std::is_same_v<Component, vec4> ||
			std::is_same_v<Component, ivec4> ||
			std::is_same_v<Component, uvec4> ||
			std::is_same_v<Component, mat4>)
			return 4;
		return 1;
	}();
	constexpr auto type = []() -> GLenum {
		if constexpr (std::is_same_v<Component, f32> ||
			std::is_same_v<Component, vec2> ||
			std::is_same_v<Component, vec3> ||
			std::is_same_v<Component, vec4> ||
			std::is_same_v<Component, mat4>)
			return GL_FLOAT;
		if constexpr (std::is_same_v<Component, u32> ||
			std::is_same_v<Component, uvec2> ||
			std::is_same_v<Component, uvec3> ||
			std::is_same_v<Component, uvec4>)
			return GL_UNSIGNED_INT;
		if constexpr (std::is_same_v<Component, i32> ||
			std::is_same_v<Component, ivec2> ||
			std::is_same_v<Component, ivec3> ||
			std::is_same_v<Component, ivec4>)
			return GL_INT;
		L.fail("Unknown vertex array component type");
	}();

	vao.bind();
	buffer.bind();
	if constexpr (type == GL_FLOAT) {
		if constexpr (std::is_same_v<Component, mat4>) {

			// Matrix version
			for (GLuint i = 0; i < 4; i += 1) {
				glEnableVertexAttribArray(index + i);
				glVertexAttribPointer(index + i, components, type, GL_FALSE,
					sizeof(T), reinterpret_cast<void*>(offset + sizeof(vec4) * i));
				if (instanced)
					glVertexAttribDivisor(index + i, 1);
				vao.attributes[index + i] = true;
			}

		} else {

			// Float scalar/array version
			glEnableVertexAttribArray(index);
			glVertexAttribPointer(index, components, type, GL_FALSE, sizeof(T),
				reinterpret_cast<void*>(offset));
			if (instanced)
				glVertexAttribDivisor(index, 1);
			vao.attributes[index] = true;

		}
	} else if constexpr (type == GL_UNSIGNED_INT || type == GL_INT) {

		// Integer scalar/array version
		glEnableVertexAttribArray(index);
		glVertexAttribIPointer(index, components, type, sizeof(T),
			reinterpret_cast<void*>(offset));
		if (instanced)
			glVertexAttribDivisor(index, 1);
		vao.attributes[index] = true;

	}

	L.debug(R"(Buffer "{}" bound to attribute {} of VAO "{}")",
		buffer.name, index, vao.name);
}

}

template<GLSLType T>
void VertexArray::setAttribute(GLuint const index, VertexBuffer<T>& buffer,
	bool const instanced)
{
	ASSERT(index > 0 || index < attributes.size());
	if constexpr (std::is_same_v<T, mat4>)
		ASSERT (index + 3 < attributes.size());
	ASSERT(id);

	detail::setVaoAttribute<T>(*this, index, buffer, 0, instanced);
}

template<TriviallyCopyable T, GLSLType U>
void VertexArray::setAttribute(GLuint const index, VertexBuffer<T>& buffer,
	U T::*field, bool const instanced)
{
	ASSERT(index > 0 || index < attributes.size());
	if constexpr (std::is_same_v<U, mat4>)
		ASSERT (index + 3 < attributes.size());
	ASSERT(id);

	detail::setVaoAttribute<U>(*this, index, buffer, offset_of(field),
		instanced);
}

template<ElementType T>
void VertexArray::setElements(ElementBuffer<T>& buffer)
{
	ASSERT(id);

	bind();
	buffer.bind();
	elementBits = sizeof(T) * 8;
}

}
